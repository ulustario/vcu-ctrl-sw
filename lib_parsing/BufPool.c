// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
#include "BufPool.h"

/*************************************************************************/
void AL_PictMngrBufPool_Deinit(AL_PictMngr_BufPool* pBufPool)
{
  for(uint8_t i = 0; i < pBufPool->uBufCnt; i++)
    AL_MemDesc_Free(&pBufPool->pBufs[i].tMD);

  Rtos_DeleteSemaphore(pBufPool->Semaphore);
  Rtos_DeleteMutex(pBufPool->Mutex);
}

/*************************************************************************/
bool AL_PictMngrBufPool_Init(AL_PictMngr_BufPool* pBufPool, uint8_t uMaxBuf, size_t zSize, AL_TAllocator* pAllocator, char const* name)
{
  Rtos_Assert(uMaxBuf <= BUFPOOL_MAX_SIZE);

  pBufPool->uBufCnt = uMaxBuf;
  IntFifo_Init(&pBufPool->tFreeIdFifo, pBufPool->vFreeIds, ARRAY_SIZE(pBufPool->vFreeIds));

  for(uint8_t i = 0; i < uMaxBuf; ++i)
  {
    AL_MemDesc_Init(&pBufPool->pBufs[i].tMD);
    pBufPool->iAccessCnt[i] = 0;
    IntFifo_Queue(&pBufPool->tFreeIdFifo, i);

    if(zSize != 0 && !AL_MemDesc_AllocNamed(&pBufPool->pBufs[i].tMD, pAllocator, zSize, name))
      goto fail_alloc;

    AL_CleanupMemory(pBufPool->pBufs[i].tMD.pVirtualAddr, pBufPool->pBufs[i].tMD.uSize);
  }

  pBufPool->Mutex = Rtos_CreateMutex();

  if(!pBufPool->Mutex)
    goto fail_alloc;

  pBufPool->Semaphore = Rtos_CreateSemaphore(uMaxBuf);

  if(!pBufPool->Semaphore)
    goto fail_alloc;

  return true;

  fail_alloc:
  AL_PictMngrBufPool_Deinit(pBufPool);

  return false;
}

/*************************************************************************/
uint8_t AL_PictMngrBufPool_GetFreeBufID(AL_PictMngr_BufPool* pBufPool)
{
  Rtos_GetSemaphore(pBufPool->Semaphore, AL_WAIT_FOREVER);
  Rtos_GetMutex(pBufPool->Mutex);

  Rtos_Assert(!IntFifo_Empty(&pBufPool->tFreeIdFifo));
  uint8_t uID = (uint8_t)IntFifo_Dequeue(&pBufPool->tFreeIdFifo);
  Rtos_Assert(pBufPool->iAccessCnt[uID] == 0);
  pBufPool->iAccessCnt[uID] = 1;
  AL_CleanupMemory(pBufPool->pBufs[uID].tMD.pVirtualAddr, pBufPool->pBufs[uID].tMD.uSize);

  Rtos_ReleaseMutex(pBufPool->Mutex);
  return uID;
}

/*************************************************************************/
void AL_PictMngrBufPool_DecrementBufID(AL_PictMngr_BufPool* pBufPool, uint8_t uID)
{
  Rtos_Assert(uID < BUFPOOL_MAX_SIZE);
  Rtos_GetMutex(pBufPool->Mutex);

  bool bFree = false;

  if(pBufPool->iAccessCnt[uID])
  {
    Rtos_AtomicDecrement(&(pBufPool->iAccessCnt[uID]));
    bFree = (pBufPool->iAccessCnt[uID] == 0);

    if(bFree)
      IntFifo_Queue(&pBufPool->tFreeIdFifo, uID);
  }

  Rtos_ReleaseMutex(pBufPool->Mutex);

  if(bFree)
    Rtos_ReleaseSemaphore(pBufPool->Semaphore);
}

/*************************************************************************/
void AL_PictMngrBufPool_IncrementBufID(AL_PictMngr_BufPool* pBufPool, uint8_t uID)
{
  Rtos_Assert(uID < BUFPOOL_MAX_SIZE);
  Rtos_GetMutex(pBufPool->Mutex);
  Rtos_AtomicIncrement(&(pBufPool->iAccessCnt[uID]));
  Rtos_ReleaseMutex(pBufPool->Mutex);
}

/*****************************************************************************/
void AL_PictMngrBufPool_Terminate(AL_PictMngr_BufPool* pBufPool)
{
  for(uint8_t i = 0; i < pBufPool->uBufCnt; ++i)
    Rtos_GetSemaphore(pBufPool->Semaphore, AL_WAIT_FOREVER);

  for(uint8_t i = 0; i < pBufPool->uBufCnt; ++i)
    Rtos_ReleaseSemaphore(pBufPool->Semaphore);
}
