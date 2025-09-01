// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common/IntFifo.h"
#include "lib_common_dec/Types.h"

#define BUFPOOL_MAX_SIZE 48
#define BUFPOOL_UNDEF_ID UINT8_MAX

/*****************************************************************************/
typedef struct AL_PictMngr_BufPool
{
  TBuffer pBufs[BUFPOOL_MAX_SIZE]; /*!< The buffer pool */
  int32_t iAccessCnt[BUFPOOL_MAX_SIZE]; /*!< Number of handles holding the buffer */
  uint8_t uBufCnt;

  int32_t vFreeIds[BUFPOOL_MAX_SIZE]; // Used by IntFifo
  IntFifo tFreeIdFifo;

  AL_MUTEX Mutex;
  AL_SEMAPHORE Semaphore;
}AL_PictMngr_BufPool;

/*****************************************************************************/
bool AL_PictMngrBufPool_Init(AL_PictMngr_BufPool* pBufPool, uint8_t uMaxBuf, size_t zSize, AL_TAllocator* pAllocator, char const* name);
void AL_PictMngrBufPool_Deinit(AL_PictMngr_BufPool* pBufPool);
AL_TIndex AL_PictMngrBufPool_GetFreeBufID(AL_PictMngr_BufPool* pBufPool);
TBuffer AL_PictMngrBufPool_GetBufFromId(AL_PictMngr_BufPool* pBufPool, AL_TIndex tID);
void AL_PictMngrBufPool_DecrementBufID(AL_PictMngr_BufPool* pBufPool, AL_TIndex tID);
void AL_PictMngrBufPool_IncrementBufID(AL_PictMngr_BufPool* pBufPool, AL_TIndex tID);
void AL_PictMngrBufPool_Terminate(AL_PictMngr_BufPool* pBufPool);
