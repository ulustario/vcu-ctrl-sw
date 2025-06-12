// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_enc/RateCtrlMeta.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/EncBuffersInternal.h"

static bool destroy(AL_TMetaData* pBaseMeta)
{
  AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)pBaseMeta;

  if(pMeta->pMVBuf != NULL)
    AL_Buffer_Unref(pMeta->pMVBuf);
  Rtos_Free(pMeta);
  return true;
}

static AL_TRateCtrlMetaData* create(AL_TAllocator* pAllocator, uint32_t uBufMVSize);

static AL_TMetaData* clone(AL_TMetaData* pBaseMeta)
{
  AL_TRateCtrlMetaData* pMeta = (AL_TRateCtrlMetaData*)pBaseMeta;
  AL_TRateCtrlMetaData* pNewMeta = NULL;

  if(pMeta->pMVBuf != NULL)
  {
    pNewMeta = create(pMeta->pMVBuf->pAllocator, AL_Buffer_GetSize(pMeta->pMVBuf));
  }
  else
  {
    pNewMeta = create(NULL, 0);
  }

  if(!pNewMeta)
    return NULL;

  pNewMeta->tRateCtrlStats = pMeta->tRateCtrlStats;
  pNewMeta->eStatCtrl = pMeta->eStatCtrl;

  if(pMeta->pMVBuf != NULL)
  {
    Rtos_Memcpy(AL_Buffer_GetData(pNewMeta->pMVBuf), AL_Buffer_GetData(pMeta->pMVBuf), AL_Buffer_GetSize(pMeta->pMVBuf));
  }

  return (AL_TMetaData*)pNewMeta;
}

static AL_TRateCtrlMetaData* create_with_buf(AL_TBuffer* pMVBuf)
{
  AL_TRateCtrlMetaData* pMeta = Rtos_Malloc(sizeof(AL_TRateCtrlMetaData));

  if(!pMeta)
    return NULL;

  pMeta->pMVBuf = pMVBuf;

  if(pMVBuf != NULL)
    AL_Buffer_Ref(pMeta->pMVBuf);

  pMeta->tMeta.eType = AL_META_TYPE_RATECTRL;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;

  pMeta->bFilled = false;

  return pMeta;
}

static AL_TRateCtrlMetaData* create(AL_TAllocator* pAllocator, uint32_t uBufMVSize)
{
  AL_TRateCtrlMetaData* pMeta;
  AL_TBuffer* pMVBuf = NULL;

  if(uBufMVSize != 0)
  {
    pMVBuf = AL_Buffer_Create_And_Allocate(pAllocator, uBufMVSize, &AL_Buffer_Destroy);

    if(!pMVBuf)
      return NULL;
  }
  pMeta = create_with_buf(pMVBuf);

  if(!pMeta)
    AL_Buffer_Destroy(pMVBuf);

  return pMeta;
}

AL_TRateCtrlMetaData* AL_RateCtrlMetaData_CustomCreate(AL_TAllocator* pAllocator, AL_ERateCtrlStatMode eStatCtrl, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec)
{
  AL_TRateCtrlMetaData* pMeta = NULL;

  if(eStatCtrl & AL_RATECTRL_STAT_MODE_MV)
  {
    uint32_t uSizeMV = AL_GetAllocSize_MV(tDim, uLog2MaxCuSize, eCodec) - MVBUFF_MV_OFFSET;
    pMeta = create(pAllocator, uSizeMV);
  }
  else
  {
    pMeta = create_with_buf(NULL);
  }

  if(pMeta)
  {
    pMeta->eStatCtrl = eStatCtrl;
  }
  return pMeta;
}

AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create(AL_TAllocator* pAllocator, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec)
{
  AL_ERateCtrlStatMode eStatCtrl = AL_RATECTRL_STAT_MODE_MV | AL_RATECTRL_STAT_MODE_DEFAULT;
  return AL_RateCtrlMetaData_CustomCreate(pAllocator, eStatCtrl, tDim, uLog2MaxCuSize, eCodec);
}

AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create_WithBuffer(AL_TBuffer* pMVBuf)
{
  return create_with_buf(pMVBuf);
}
