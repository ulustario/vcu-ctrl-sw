// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "SearchDecUnit.h"
#include "lib_common/Nuts.h"
#include "lib_common/SEI.h"
#include "lib_common/AvcUtils.h"
#include "lib_common/HevcUtils.h"
#include "lib_rtos/lib_rtos.h"

/*****************************************************************************/
static bool isAud(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_AUD;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_AUD;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isVcl(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AL_AVC_IsVcl(eNut);
  case AL_CODEC_HEVC:
    return AL_HEVC_IsVcl(eNut);
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isEosOrEob(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return (eNut == AL_AVC_NUT_EOS) || (eNut == AL_AVC_NUT_EOB);
  case AL_CODEC_HEVC:
    return (eNut == AL_HEVC_NUT_EOS) || (eNut == AL_HEVC_NUT_EOB);
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isFd(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_FD;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_FD;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool isPrefixSei(AL_ECodec eCodec, AL_ENut eNut)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return eNut == AL_AVC_NUT_PREFIX_SEI;
  case AL_CODEC_HEVC:
    return eNut == AL_HEVC_NUT_PREFIX_SEI;
  default:
    (void)eNut;
    return false;
  }
}

/*****************************************************************************/
static bool checkSeiUUID(uint8_t const* pBufs, AL_TNal const* pNal, AL_ECodec eCodec, int iTotalSize)
{
  (void)eCodec;
  int iTotalUUIDSize = 26;

  if(eCodec == AL_CODEC_AVC)
    iTotalUUIDSize = 25;

  if((int)pNal->uSize != iTotalUUIDSize)
    return false;

  int iStart = 7;

  if(eCodec == AL_CODEC_AVC)
    iStart = 6;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);

  for(int i = 0; i < iSize; i++)
  {
    int iPosition = (pNal->tStartCode.uPosition + iStart + i) % iTotalSize;

    if(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID[i] != pBufs[iPosition])
      return false;
  }

  return true;
}

/*****************************************************************************/
static int getNumSliceInSei(uint8_t* pBufs, AL_TNal* pNal, AL_ECodec eCodec, int iTotalSize)
{
  (void)eCodec;
  Rtos_Assert(checkSeiUUID(pBufs, pNal, eCodec, iTotalSize));
  int iStart = 7;

  if(eCodec == AL_CODEC_AVC)
    iStart = 6;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);
  int iPosition = (pNal->tStartCode.uPosition + iStart + iSize) % iTotalSize;
  return pBufs[iPosition];
}

/*****************************************************************************/
void AL_SearchDecUnit_Init(AL_TDecUnitSearchCtx* pCtx, AL_ECodec eCodec, AL_EDecUnit eDecUnit, AL_TNal* pNals, int iMaxNal)
{
  pCtx->eCodec = eCodec;
  pCtx->bSubFrameUnit = (eDecUnit == AL_VCL_NAL_UNIT);

  pCtx->pNals = pNals;
  pCtx->iMaxNal = iMaxNal;

  pCtx->pStream = NULL;
  pCtx->uStreamBufSize = 0;

  pCtx->iNalCount = 0;
  pCtx->iCurNalStreamOffset = 0;

  pCtx->bEOS = false;

}

/*****************************************************************************/
void AL_SearchDecUnit_Reset(AL_TDecUnitSearchCtx* pCtx)
{
  pCtx->iNalCount = 0;
  pCtx->iCurNalStreamOffset = 0;
}

/*****************************************************************************/
void AL_SearchDecUnit_SetStream(AL_TDecUnitSearchCtx* pCtx, uint8_t* pStream, uint32_t uSize)
{
  pCtx->pStream = pStream;
  pCtx->uStreamBufSize = uSize;
}

/*****************************************************************************/
uint32_t AL_SearchDecUnit_GetStorageSize(AL_TDecUnitSearchCtx* pCtx)
{
  return pCtx->iMaxNal - pCtx->iNalCount;
}

/*****************************************************************************/
static bool isFirstSliceStatusAvailable(int iSize, int iNalHdrSize)
{
  return iSize > iNalHdrSize;
}

/******************************************************************************/
static int NalHeaderSize(AL_ECodec eCodec)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AL_AVC_NAL_HDR_SIZE;
  case AL_CODEC_HEVC:
    return AL_HEVC_NAL_HDR_SIZE;
  default:
    return -1;
  }
}

/* should only be used when the position is right after the nal header */
/*****************************************************************************/
static bool isFirstSlice(uint8_t* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return (pBuf[uPos] & 0x80) != 0;
}

/*****************************************************************************/
static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int iNalHdrSize = NalHeaderSize(eCodec);
  Rtos_Assert(iNalHdrSize);
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
#define IS_START_CODE(pBuf, uSize, uPos) ( \
    (pBuf[uPos % uSize] == 0x00) && \
    (pBuf[(uPos + 1) % uSize] == 0x00) && \
    (pBuf[(uPos + 2) % uSize] == 0x01))

/*****************************************************************************/
static bool isFirstSliceNAL(AL_TDecUnitSearchCtx* pCtx, AL_TNal* pNal)
{
  uint8_t* pBuf = pCtx->pStream;
  uint32_t uPos = pNal->tStartCode.uPosition;
  uint32_t uSize = pCtx->uStreamBufSize;

  Rtos_Assert(IS_START_CODE(pBuf, uSize, uPos));

  uPos = skipNalHeader(uPos, pCtx->eCodec, uSize);
  return isFirstSlice(pBuf, uPos);
}

/*****************************************************************************/
int AL_SearchDecUnit_GetLastVCL(AL_TDecUnitSearchCtx* pCtx)
{
  int iLastVclNalInAU = -1;

  for(int iNal = pCtx->iNalCount - 1; iNal >= 0; --iNal)
  {
    AL_ENut eNUT = pCtx->pNals[iNal].tStartCode.uNUT;

    if(isVcl(pCtx->eCodec, eNUT))
    {
      iLastVclNalInAU = iNal;
      break;
    }
  }

  return iLastVclNalInAU;
}

/*****************************************************************************/
bool AL_SearchDecUnit_GetNextUnit(AL_TDecUnitSearchCtx* pCtx, int* pNalCount, int* pLastVclNalInDecodingUnit)
{
  *pNalCount = 0;
  (void)pLastVclNalInDecodingUnit;

  if(pCtx->iNalCount == 0)
    return false;

  bool bVclNalSeen = false;
  int iNalFound = 0;

  for(int iNal = 0; iNal < pCtx->iNalCount; ++iNal)
  {
    AL_TNal* pNal = &pCtx->pNals[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(iNal > 0)
      iNalFound++;

    if(!isVcl(pCtx->eCodec, eNUT))
    {
      if(isAud(pCtx->eCodec, eNUT) || isEosOrEob(pCtx->eCodec, eNUT))
      {
        if(bVclNalSeen)
        {
          *pNalCount = iNalFound;
          return true;
        }
      }

      if(isPrefixSei(pCtx->eCodec, eNUT) && checkSeiUUID(pCtx->pStream, pNal, pCtx->eCodec, pCtx->uStreamBufSize))
      {
        pCtx->iNumSlicesRemaining = getNumSliceInSei(pCtx->pStream, pNal, pCtx->eCodec, pCtx->uStreamBufSize);
        Rtos_Assert(pCtx->iNumSlicesRemaining > 0);
      }

      if(isFd(pCtx->eCodec, eNUT) && pCtx->bSubFrameUnit)
      {
        bool bIsLastSlice = pCtx->iNumSlicesRemaining == 1;

        if(bIsLastSlice && bVclNalSeen)
        {
          *pNalCount = iNalFound + 1;
          pCtx->iNumSlicesRemaining = 0;
          return true;
        }
      }

    }

    if(isVcl(pCtx->eCodec, eNUT))
    {
      int iNalHdrSize = NalHeaderSize(pCtx->eCodec);
      Rtos_Assert(iNalHdrSize > 0);

      if(isFirstSliceStatusAvailable(pNal->uSize, iNalHdrSize))
      {
        bool bIsFirstSlice = isFirstSliceNAL(pCtx, pNal);

        if(bVclNalSeen)
        {
          if(bIsFirstSlice)
          {
            *pNalCount = iNalFound;
            return true;
          }

          if(pCtx->bSubFrameUnit)
          {
            pCtx->iNumSlicesRemaining--;
            *pNalCount = iNalFound;
            int const iIsNotLastSlice = -1;
            *pLastVclNalInDecodingUnit = iIsNotLastSlice;
            return true;
          }
        }
      }

      bVclNalSeen = true;
      *pLastVclNalInDecodingUnit = iNal;
    }
  }

  if(pCtx->bEOS)
  {
    if(bVclNalSeen)
    {
      *pNalCount = pCtx->iNalCount;
      return true;
    }

    AL_SearchDecUnit_ConsumeNals(pCtx, pCtx->iNalCount);
  }

  return false;
}

/*****************************************************************************/
int AL_SearchDecUnit_GetCurNalCount(AL_TDecUnitSearchCtx* pCtx)
{
  return pCtx->iNalCount;
}

/*****************************************************************************/
void AL_SearchDecUnit_Update(AL_TDecUnitSearchCtx* pCtx, AL_TStartCode* pSC, int iNumSC, uint32_t uLastByte, bool bEOS)
{
  AL_TNal* dst = pCtx->pNals;

  if(pCtx->iNalCount && iNumSC)
    dst[pCtx->iNalCount - 1].uSize = DeltaPosition(dst[pCtx->iNalCount - 1].tStartCode.uPosition, pSC[0].uPosition, pCtx->uStreamBufSize);

  for(int i = 0; i < iNumSC; i++)
  {
    dst[pCtx->iNalCount].tStartCode = pSC[i];

    if(i + 1 == iNumSC)
      dst[pCtx->iNalCount].uSize = DeltaPosition(pSC[i].uPosition, uLastByte, pCtx->uStreamBufSize);
    else
      dst[pCtx->iNalCount].uSize = DeltaPosition(pSC[i].uPosition, pSC[i + 1].uPosition, pCtx->uStreamBufSize);

    pCtx->iNalCount++;
  }

  pCtx->bEOS = bEOS;
}

/*****************************************************************************/
void AL_SearchDecUnit_ConsumeNals(AL_TDecUnitSearchCtx* pCtx, int iNumNal)
{
  if(iNumNal)
    pCtx->iCurNalStreamOffset = (pCtx->pNals[iNumNal - 1].tStartCode.uPosition + pCtx->pNals[iNumNal - 1].uSize) % pCtx->uStreamBufSize;

  pCtx->iNalCount -= iNumNal;
  Rtos_Memmove(pCtx->pNals, pCtx->pNals + iNumNal, pCtx->iNalCount * sizeof(AL_TNal));
}

/*****************************************************************************/
int AL_SearchDecUnit_GetCurOffset(AL_TDecUnitSearchCtx* pCtx)
{
  return pCtx->iCurNalStreamOffset;
}
