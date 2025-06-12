// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/MemDesc.h"
#include "lib_common_dec/StartCodeParam.h"
#include "lib_common_dec/DecSynchro.h"

/*****************************************************************************/
typedef struct AL_TDecUnitSearchCtx
{
  uint8_t const* pStream;
  uint32_t uStreamBufSize;

  AL_ECodec eCodec;
  bool bSubFrameUnit;

  AL_TNal* pNals;
  int32_t iMaxNal;
  int32_t iNalCount;

  bool bEOS;

  int32_t iCurNalStreamOffset;

  int32_t iNumSlicesRemaining;
}AL_TDecUnitSearchCtx;

/*****************************************************************************/
static inline size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  return uSize + uSecondPos - uFirstPos;
}

/*****************************************************************************
   \brief Initializes a decoder-unit searcher. Using a stream and start-codes
          positions, provided by external means, it allows to split the stream
          into decoding units (frames or slices, depending on the decoding mode).
   \param[in] pCtx Decoder-unit searcher context
   \param[in] eDecUnit Indicate if we decode in frame unit or in slice unit.
   \param[in] eCodec Codec currently being parsed
   \param[in] pNals Will be used by the decoder-unit searcher to store recorded
                    Nals
   \param[in] iMaxNal Maximum number of Nals that can be stored in pNals
*****************************************************************************/
void AL_SearchDecUnit_Init(AL_TDecUnitSearchCtx* pCtx, AL_ECodec eCodec, AL_EDecUnit eDecUnit, AL_TNal* pNals, int32_t iMaxNal);

/*****************************************************************************
   \brief Set the stream that we want to parse
   \param[in] pCtx Decoder-unit searcher context
   \param[in] pStream Pointer to the stream to parse
   \param[in] uSize Size of the stream buffer
*****************************************************************************/
void AL_SearchDecUnit_SetStream(AL_TDecUnitSearchCtx* pCtx, uint8_t const* pStream, uint32_t uSize);

/*****************************************************************************
   \brief Return the number of Nals currently in the decoder-unit searcher
   \param[in] pCtx Decoder-unit searcher context
   \returns Number of Nals in the decoder-unit searcher
*****************************************************************************/
int32_t AL_SearchDecUnit_GetCurrentNalCount(AL_TDecUnitSearchCtx const* pCtx);

/*****************************************************************************
   \brief Return the number of Nals that can still be added in the decoder-unit
          searcher
   \param[in] pCtx Decoder-unit searcher context
   \returns Number of Nals that can be added in the decoder-unit searcher
*****************************************************************************/
uint32_t AL_SearchDecUnit_GetFreeNalCount(AL_TDecUnitSearchCtx const* pCtx);

/*****************************************************************************
   \brief Indicates to the decoder-unit searcher new Nal positions in the bitstream
          it has been provided
   \param[in] pCtx Decoder-unit searcher context
   \param[in] pNalSC The list of start codes for the new Nals
   \param[in] iNumNalSC The number of Nals to add
   \param[in] uLastByte Position in the stream of the last-byte of the last added Nal.
                        This might not be the real last-byte of the Nal as the last
                        added Nal might be incomplete at this time.
   \param[in] bEOS Indicates if those added Nals are the last ones of the stream.
*****************************************************************************/
void AL_SearchDecUnit_AddNals(AL_TDecUnitSearchCtx* pCtx, AL_TStartCode const* pNalSC, int32_t iNumNalSC, uint32_t uLastByte, bool bEOS);

/*****************************************************************************
   \brief Get the next decoding unit to decode
   \param[in] pCtx Decoder-unit searcher context
   \param[out] pNalCount Number of Nals in the next decoding unit
   \param[out] pLastVclNalInAccessUnit Index of the last VCL Nals in the next
                                       decoding unit. Can be -1 if it is unknown,
                                       like when we decode in slice unit and its
                                       not the last slice. Note: VCL mean video
                                       content, ie slices for example, not
                                       headers like SPS/PPS...
   \returns True if a decoding unit has been found, false otherwise
*****************************************************************************/
bool AL_SearchDecUnit_GetNextUnit(AL_TDecUnitSearchCtx* pCtx, int32_t* pNalCount, int32_t* pLastVclNalInAccessUnit);

/*****************************************************************************
   \brief Get the index of the last VCL Nal in the decoder-unit searcher
   \param[in] pCtx Decoder-unit searcher context
   \returns The index of the last VCL Nal or -1 if none
*****************************************************************************/
int32_t AL_SearchDecUnit_GetLastVCL(AL_TDecUnitSearchCtx const* pCtx);

/*****************************************************************************
   \brief Get the offset in the stream to the current Nal.
   \param[in] pCtx Decoder-unit searcher context
*****************************************************************************/
int32_t AL_SearchDecUnit_GetCurrentStreamOffset(AL_TDecUnitSearchCtx const* pCtx);

/*****************************************************************************
   \brief Consume some Nals of the decoder-unit searcher (generally after they
          have been extracted as a units to decode), and move stream offset to
          the following Nals.
   \param[in] pCtx Decoder-unit searcher context
   \param[in] iNumNal Number of Nals to remove
*****************************************************************************/
void AL_SearchDecUnit_ConsumeNals(AL_TDecUnitSearchCtx* pCtx, int32_t iNumNal);

/*****************************************************************************
   \brief Remove all Nals from the decoder-unit searcher and move back to stream
          buffer start.
   \param[in] pCtx Decoder-unit searcher context
*****************************************************************************/
void AL_SearchDecUnit_ResetNals(AL_TDecUnitSearchCtx* pCtx);
