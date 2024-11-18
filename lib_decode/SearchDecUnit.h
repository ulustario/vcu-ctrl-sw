// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/MemDesc.h"
#include "lib_common_dec/StartCodeParam.h"
#include "lib_common_dec/DecSynchro.h"

/*****************************************************************************/
typedef struct AL_TDecUnitSearchCtx
{
  uint8_t* pStream;
  uint32_t uStreamBufSize;

  AL_ECodec eCodec;
  bool bSubFrameUnit;

  AL_TNal* pNals;
  int iMaxNal;
  int iNalCount;

  bool bEOS;

  int iCurNalStreamOffset;

  int iNumSlicesRemaining;
}AL_TDecUnitSearchCtx;

/*****************************************************************************/
static inline size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  return uSize + uSecondPos - uFirstPos;
}

/*****************************************************************************/
void AL_SearchDecUnit_Init(AL_TDecUnitSearchCtx* pCtx, AL_ECodec eCodec, AL_EDecUnit eDecUnit, AL_TNal* pNals, int iMaxNal);

void AL_SearchDecUnit_Reset(AL_TDecUnitSearchCtx* pCtx);

void AL_SearchDecUnit_SetStream(AL_TDecUnitSearchCtx* pCtx, uint8_t* pStream, uint32_t uSize);

uint32_t AL_SearchDecUnit_GetStorageSize(AL_TDecUnitSearchCtx* pCtx);

bool AL_SearchDecUnit_GetNextUnit(AL_TDecUnitSearchCtx* pCtx, int* pNalCount, int* iLastVclNalInDecodingUnit);

int AL_SearchDecUnit_GetLastVCL(AL_TDecUnitSearchCtx* pCtx);

int AL_SearchDecUnit_GetCurNalCount(AL_TDecUnitSearchCtx* pCtx);

void AL_SearchDecUnit_Update(AL_TDecUnitSearchCtx* pCtx, AL_TStartCode* pSC, int iNumSC, uint32_t uLastByte, bool bEOS);

void AL_SearchDecUnit_ConsumeNals(AL_TDecUnitSearchCtx* pCtx, int iNumNal);

int AL_SearchDecUnit_GetCurOffset(AL_TDecUnitSearchCtx* pCtx);
