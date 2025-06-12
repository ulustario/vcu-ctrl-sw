// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferCircMeta.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufConst.h"
#include "lib_common/Planes.h"
#include "lib_common/MemDesc.h"
#include "lib_rtos/lib_rtos.h"

/*****************************************************************************
   \brief Generic Buffer
*****************************************************************************/
typedef struct TBuffer
{
  AL_TMemDesc tMD; /*!< Memory descriptor associated to the buffer */
}TBuffer;

/*****************************************************************************
   \brief Buffer with Motion Vectors content
*****************************************************************************/
typedef TBuffer TBufferMV;

/*****************************************************************************
   \brief Circular Buffer
*****************************************************************************/
typedef struct AL_TCircBuffer
{
  AL_TMemDesc tMD; /*!< Memory descriptor associated to the buffer */

  int32_t iOffset; /*!< Initial Offset in Circular Buffer */
  int32_t iAvailSize; /*!< Avail Space in Circular Buffer */
}AL_TCircBuffer;

static inline void CircBuffer_ConsumeUpToOffset(AL_TBuffer* stream, int32_t iNewOffset)
{
  AL_TCircMetaData* pCircMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(stream, AL_META_TYPE_CIRCULAR);

  if(iNewOffset < pCircMeta->iOffset)
    pCircMeta->iAvailSize -= iNewOffset + AL_Buffer_GetSize(stream) - pCircMeta->iOffset;
  else
    pCircMeta->iAvailSize -= iNewOffset - pCircMeta->iOffset;
  pCircMeta->iOffset = iNewOffset;

  Rtos_Assert(pCircMeta->iAvailSize >= 0);
}

static inline void CircBuffer_Init(AL_TCircBuffer* pBuf)
{
  pBuf->iOffset = 0;
  pBuf->iAvailSize = 0;
}

typedef AL_TDimension AL_TPitch;

uint32_t AL_GetAllocSize_Frame_Full(AL_TDimension tDim, AL_TPicFormat const* pPicFormat, AL_TPitch tPitchDim, int32_t iRoundVal);

uint32_t AL_GetAllocSize_Frame_PixPlane(AL_TPicFormat const* pPicFormat, AL_TPitch tPitchDim, AL_EPlaneId ePlaneId);

void AL_CleanupMemory(void* pDst, size_t uSize);
