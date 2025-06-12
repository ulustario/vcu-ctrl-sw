// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "I_PictMngr.h"

#include <limits.h>
#include "lib_common/PicFormat.h"
#include "lib_common_dec/DecOutputSettingsInternal.h"
#include "lib_common/Error.h"
#include "lib_common/PixMapBufferInternal.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_common/DisplayInfoMeta.h"
#include "lib_decode/lib_decode.h"
#include "lib_rtos/lib_rtos.h"

/*************************************************************************/
static AL_TBuffer* sRecBuffers_GetDisplayBuffer(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;

  return tRecBuffers->pFrame;
}

/*************************************************************************/
static bool sRecBuffers_AreNull(AL_TRecBuffers const* tRecBuffers)
{
  bool bNull = NULL == tRecBuffers->pFrame;

  return bNull;
}

/*************************************************************************/
static bool sRecBuffers_AreNotNull(AL_TRecBuffers const* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  bool bNotNull = tRecBuffers->pFrame;

  return bNotNull;
}

/*************************************************************************/
static void sRecBuffers_Reset(AL_TRecBuffers* tRecBuffers)
{
  tRecBuffers->pFrame = NULL;
}

/*************************************************************************/
static void sRecBuffers_CleanUp(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;

  AL_Buffer_Cleanup(tRecBuffers->pFrame);

}

/*************************************************************************/
static void sRecBuffers_Release(AL_TRecBuffers* tRecBuffers, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  AL_Buffer_Unref(tRecBuffers->pFrame);
}

/*************************************************************************/
static bool sRecBuffers_HasBuf(AL_TRecBuffers const* tRecBuffers, AL_TBuffer const* pBuf, AL_TFrmBufPool const* pFrmBufPool)
{
  (void)pFrmBufPool;
  bool bHasBuf = tRecBuffers->pFrame == pBuf;

  return bHasBuf;
}

/*************************************************************************/
static int32_t sFrmBufPool_GetFrameIDFromBuf(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  for(int32_t i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(sRecBuffers_HasBuf(&pFrmBufPool->array[i].tRecBuffers, pBuf, pFrmBufPool))
    {
      Rtos_ReleaseMutex(pFrmBufPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return -1;
}

/*************************************************************************/
static int32_t sFrmBufPool_GetFrameIDFromDisplay(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  for(int32_t i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(sRecBuffers_GetDisplayBuffer(&pFrmBufPool->array[i].tRecBuffers, pFrmBufPool) == pDisplayBuf)
    {
      Rtos_ReleaseMutex(pFrmBufPool->Mutex);
      return i;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return -1;
}

/*************************************************************************/
static bool sFrmBufPoolFifo_IsInFifo(AL_TFrmBufPool const* pFrmBufPool, AL_TBuffer const* pDisplayBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  for(int32_t iCur = pFrmBufPool->iFifoHead; iCur != -1; iCur = pFrmBufPool->array[iCur].iNext)
  {
    if(sRecBuffers_GetDisplayBuffer(&pFrmBufPool->array[iCur].tRecBuffers, pFrmBufPool) == pDisplayBuf)
    {
      Rtos_ReleaseMutex(pFrmBufPool->Mutex);
      return true;
    }
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return false;
}

/*************************************************************************/
static void AddBufferToFifo(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, AL_TRecBuffers const* pRecBuffers)
{
  Rtos_Memcpy(&pFrmBufPool->array[iFrameID].tRecBuffers, pRecBuffers, sizeof(AL_TRecBuffers));

  if(pFrmBufPool->iFifoTail == -1 && pFrmBufPool->iFifoHead == -1)
    pFrmBufPool->iFifoHead = iFrameID;
  else
    pFrmBufPool->array[pFrmBufPool->iFifoTail].iNext = iFrameID;

  pFrmBufPool->iFifoTail = iFrameID;

  pFrmBufPool->array[iFrameID].iAccessCnt = 0;
  pFrmBufPool->array[iFrameID].bWillBeOutputted = false;

  ++pFrmBufPool->iBufNumber;
}

/*************************************************************************/
static bool sFrmBufPoolFifo_PushBack(AL_TFrmBufPool* pFrmBufPool, AL_TRecBuffers const* pRecBuffers)
{
  bool const bRecNotNull = sRecBuffers_AreNotNull(pRecBuffers, pFrmBufPool);
  (void)bRecNotNull;
  Rtos_Assert(bRecNotNull);

  Rtos_GetMutex(pFrmBufPool->Mutex);

  int32_t iPoolIdx = 0;

  while(iPoolIdx < FRM_BUF_POOL_SIZE && (!sRecBuffers_AreNull(&pFrmBufPool->array[iPoolIdx].tRecBuffers) ||
                                         (pFrmBufPool->array[iPoolIdx].iNext != -1) ||
                                         (pFrmBufPool->array[iPoolIdx].iAccessCnt != -1) ||
                                         pFrmBufPool->array[iPoolIdx].bWillBeOutputted
                                         ))
    iPoolIdx++;

  if(iPoolIdx < FRM_BUF_POOL_SIZE)
  {
    AddBufferToFifo(pFrmBufPool, iPoolIdx, pRecBuffers);
    Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return iPoolIdx < FRM_BUF_POOL_SIZE;
}

/*************************************************************************/
static int32_t RemoveBufferFromFifo(AL_TFrmBufPool* pFrmBufPool)
{
  int32_t const iFrameID = pFrmBufPool->iFifoHead;
  --pFrmBufPool->iBufNumber;

  pFrmBufPool->iFifoHead = pFrmBufPool->array[pFrmBufPool->iFifoHead].iNext;

  pFrmBufPool->array[iFrameID].iNext = -1;
  pFrmBufPool->array[iFrameID].iAccessCnt = 1;

  if(pFrmBufPool->iFifoHead == -1)
    pFrmBufPool->iFifoTail = pFrmBufPool->iFifoHead;
  return iFrameID;
}

/*************************************************************************/
static int32_t sFrmBufPoolFifo_Pop(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetSemaphore(pFrmBufPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_GetMutex(pFrmBufPool->Mutex);

  if(pFrmBufPool->isDecommited)
  {
    Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return UndefID;
  }

  Rtos_Assert(pFrmBufPool->iBufNumber > 0);
  Rtos_Assert(pFrmBufPool->iFifoHead != -1);
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pFrmBufPool->array[pFrmBufPool->iFifoHead].tRecBuffers, pFrmBufPool);
  (void)bRecNotNull;
  Rtos_Assert(bRecNotNull);
  Rtos_Assert(pFrmBufPool->array[pFrmBufPool->iFifoHead].iAccessCnt == 0);
  Rtos_Assert(pFrmBufPool->array[pFrmBufPool->iFifoHead].bWillBeOutputted == false);

  sRecBuffers_CleanUp(&pFrmBufPool->array[pFrmBufPool->iFifoHead].tRecBuffers, pFrmBufPool);
  pFrmBufPool->array[pFrmBufPool->iFifoHead].eError = AL_SUCCESS;

  int32_t const iFrameID = RemoveBufferFromFifo(pFrmBufPool);

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return iFrameID;
}

/*************************************************************************/
static void sFrmBufPoolFifo_Decommit(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  pFrmBufPool->isDecommited = true;
  Rtos_ReleaseSemaphore(pFrmBufPool->Semaphore);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_RemoveID(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pFrame->tRecBuffers, pFrmBufPool);
  (void)bRecNotNull;
  Rtos_Assert(bRecNotNull);
  Rtos_Assert(pFrame->iNext == -1);
  Rtos_Assert(pFrame->iAccessCnt == 0);

  sRecBuffers_Reset(&pFrame->tRecBuffers);
  pFrame->iAccessCnt = -1;
  pFrame->bWillBeOutputted = false;
  pFrame->iNext = -1;

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static AL_TRecBuffers* sFrmBufPool_GetBufferFromID(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID)
{
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  bool const bRecNotNull = sRecBuffers_AreNotNull(&pFrmBufPool->array[iFrameID].tRecBuffers, pFrmBufPool);
  (void)bRecNotNull;
  Rtos_Assert(bRecNotNull);
  return &pFrmBufPool->array[iFrameID].tRecBuffers;
}

/*************************************************************************/
static void sFrmBufPoolFifo_Init(AL_TFrmBufPool* pFrmBufPool)
{
  for(int32_t i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    sRecBuffers_Reset(&pFrmBufPool->array[i].tRecBuffers);
    pFrmBufPool->array[i].iNext = -1;
    pFrmBufPool->array[i].iAccessCnt = -1;
    pFrmBufPool->array[i].bWillBeOutputted = false;
  }

  pFrmBufPool->iFifoHead = -1;
  pFrmBufPool->iFifoTail = -1;
}

/*************************************************************************/
static bool sFrmBufPool_Init(AL_TFrmBufPool* pFrmBufPool, AL_TAllocator* pAllocator, bool bHasSecondOutputFrame)
{
  (void)pAllocator;
  (void)bHasSecondOutputFrame;

  pFrmBufPool->Mutex = Rtos_CreateMutex();

  if(!pFrmBufPool->Mutex)
    goto fail_alloc_mutex;

  pFrmBufPool->Semaphore = Rtos_CreateSemaphore(0);

  if(!pFrmBufPool->Semaphore)
    goto fail_alloc_sem_free;

  pFrmBufPool->iBufNumber = 0;
  sFrmBufPoolFifo_Init(pFrmBufPool);

  return true;
  fail_alloc_sem_free:
  Rtos_DeleteMutex(pFrmBufPool->Mutex);
  fail_alloc_mutex:
  return false;
}

/*************************************************************************/
static void sFrmBufPool_EnsureRecBufferAreRemoved(AL_TFrmBufPool* pFrmBufPool)
{
  for(int32_t i = 0; i < FRM_BUF_POOL_SIZE; i++)
  {
    if(pFrmBufPool->array[i].iAccessCnt == 0)
    {
      AL_TRecBuffers tBuffers;
      Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBufferFromID(pFrmBufPool, i), sizeof(tBuffers));
      sFrmBufPool_RemoveID(pFrmBufPool, i);
      sRecBuffers_Release(&tBuffers, pFrmBufPool);
    }

    Rtos_Assert(pFrmBufPool->array[i].iAccessCnt == -1);
  }
}

/*************************************************************************/
static void sFrmBufPool_Deinit(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_Assert(pFrmBufPool->iFifoHead == -1);
  Rtos_Assert(pFrmBufPool->iFifoTail == -1);

  sFrmBufPool_EnsureRecBufferAreRemoved(pFrmBufPool);

  Rtos_DeleteSemaphore(pFrmBufPool->Semaphore);
  Rtos_DeleteMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_DecrementBufID(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, bool bForceOutput)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];
  Rtos_Assert(pFrame->iAccessCnt >= 1);
  pFrame->iAccessCnt--;

  if(pFrame->iAccessCnt == 0 && !pFrame->bOutEarly)
  {
    bool bWillBeOutputted = pFrame->bWillBeOutputted;
    AL_TRecBuffers tBuffers;
    Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID), sizeof(tBuffers));
    sFrmBufPool_RemoveID(pFrmBufPool, iFrameID);

    if((!bWillBeOutputted) || bForceOutput)
    {
      bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pFrmBufPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pFrmBufPool)) == false;
      (void)bNotInFifo;
      Rtos_Assert(bNotInFifo);
      sFrmBufPoolFifo_PushBack(pFrmBufPool, &tBuffers);
    }
    else
      sRecBuffers_Release(&tBuffers, pFrmBufPool);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_OutputBufID(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];
  Rtos_Assert(pFrame->bWillBeOutputted == false);
  pFrame->bWillBeOutputted = true;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
static void sFrmBufPool_IncrementBufID(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];
  Rtos_Assert(pFrame->iAccessCnt >= 0);
  pFrame->iAccessCnt++;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*****************************************************************************/
static void sFrmBufPool_Terminate(AL_TFrmBufPool* pFrmBufPool)
{
  (void)pFrmBufPool; // Nothing to do
}

/*************************************************************************/
static bool sMvBufPool_Init(AL_TMvBufPool* pMvBufPool, int32_t iMaxBuf)
{
  Rtos_Assert(iMaxBuf <= MAX_DPB_SIZE);

  for(int32_t i = 0; i < iMaxBuf; ++i)
  {
    pMvBufPool->pFreeIDs[i] = i;
    pMvBufPool->iAccessCnt[i] = 0;
    AL_CleanupMemory(pMvBufPool->pMvBufs[i].tMD.pVirtualAddr, pMvBufPool->pMvBufs[i].tMD.uSize);
    AL_CleanupMemory(pMvBufPool->pPocBufs[i].tMD.pVirtualAddr, pMvBufPool->pPocBufs[i].tMD.uSize);
  }

  pMvBufPool->iBufCnt = iMaxBuf;

  for(int32_t i = iMaxBuf; i < MAX_DPB_SIZE; ++i)
  {
    pMvBufPool->pFreeIDs[i] = UndefID;
    pMvBufPool->iAccessCnt[i] = 0;
  }

  pMvBufPool->iFreeCnt = iMaxBuf;
  pMvBufPool->Mutex = Rtos_CreateMutex();

  if(!pMvBufPool->Mutex)
    goto fail_alloc_mutex;

  pMvBufPool->Semaphore = Rtos_CreateSemaphore(iMaxBuf);

  if(!pMvBufPool->Semaphore)
    goto fail_alloc_sem;

  return true;
  fail_alloc_sem:
  Rtos_DeleteMutex(pMvBufPool->Mutex);
  fail_alloc_mutex:
  return false;
}

/*************************************************************************/
static void sMvBufPool_Deinit(AL_TMvBufPool* pMvBufPool)
{
  Rtos_DeleteSemaphore(pMvBufPool->Semaphore);
  Rtos_DeleteMutex(pMvBufPool->Mutex);
}

/*************************************************************************/
static uint8_t sMvBufPool_GetFreeBufID(AL_TMvBufPool* pMvBufPool)
{
  Rtos_GetSemaphore(pMvBufPool->Semaphore, AL_WAIT_FOREVER);

  Rtos_GetMutex(pMvBufPool->Mutex);
  uint8_t uMvID = pMvBufPool->pFreeIDs[--pMvBufPool->iFreeCnt];
  Rtos_Assert(pMvBufPool->iAccessCnt[uMvID] == 0);
  pMvBufPool->iAccessCnt[uMvID] = 1;

  AL_CleanupMemory(pMvBufPool->pMvBufs[uMvID].tMD.pVirtualAddr, pMvBufPool->pMvBufs[uMvID].tMD.uSize);

  Rtos_ReleaseMutex(pMvBufPool->Mutex);

  return uMvID;
}

/*************************************************************************/
static void sMvBufPool_DecrementBufID(AL_TMvBufPool* pMvBufPool, uint8_t uMvID)
{
  Rtos_Assert(uMvID < MAX_DPB_SIZE);

  Rtos_GetMutex(pMvBufPool->Mutex);

  bool bFree = false;

  if(pMvBufPool->iAccessCnt[uMvID])
  {
    bFree = (--pMvBufPool->iAccessCnt[uMvID] == 0);

    if(bFree)
      pMvBufPool->pFreeIDs[pMvBufPool->iFreeCnt++] = uMvID;
  }

  Rtos_ReleaseMutex(pMvBufPool->Mutex);

  if(bFree)
    Rtos_ReleaseSemaphore(pMvBufPool->Semaphore);
}

/*************************************************************************/
static void sMvBufPool_IncrementBufID(AL_TMvBufPool* pMvBufPool, int32_t iMvID)
{
  Rtos_Assert(iMvID < FRM_BUF_POOL_SIZE);
  Rtos_GetMutex(pMvBufPool->Mutex);
  Rtos_AtomicIncrement(&(pMvBufPool->iAccessCnt[iMvID]));
  Rtos_ReleaseMutex(pMvBufPool->Mutex);
}

/*****************************************************************************/
static void sMvBufPool_Terminate(AL_TMvBufPool* pMvBufPool)
{
  for(int32_t i = 0; i < pMvBufPool->iBufCnt; ++i)
    Rtos_GetSemaphore(pMvBufPool->Semaphore, AL_WAIT_FOREVER);

  for(int32_t i = 0; i < pMvBufPool->iBufCnt; ++i)
    Rtos_ReleaseSemaphore(pMvBufPool->Semaphore);
}

/*************************************************************************/
static void sPictMngr_DecrementFrmBuf(void* pUserParam, int32_t iFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID, pCtx->bForceOutput);
}

/*************************************************************************/
static void sPictMngr_IncrementFrmBuf(void* pUserParam, int32_t iFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_IncrementBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_OutputFrmBuf(void* pUserParam, int32_t iFrameID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sFrmBufPool_OutputBufID(&pCtx->FrmBufPool, iFrameID);
}

/*************************************************************************/
static void sPictMngr_IncrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sMvBufPool_IncrementBufID(&pCtx->MvBufPool, uMvID);
}

/*************************************************************************/
static void sPictMngr_DecrementMvBuf(void* pUserParam, uint8_t uMvID)
{
  Rtos_Assert(pUserParam);
  AL_TPictMngrCtx* pCtx = (AL_TPictMngrCtx*)pUserParam;

  if(pCtx->bCompleteInit)
    sMvBufPool_DecrementBufID(&pCtx->MvBufPool, uMvID);
}

/*****************************************************************************/
static bool CheckPictMngrInitParameter(AL_TPictMngrParam const* pParam)
{
  if(pParam->iSizeMV < 0)
    return false;

  if(pParam->iNumMV < 0)
    return false;

  if(pParam->iNumDPBRef < 0)
    return false;

  if(pParam->iNumMV < pParam->iNumDPBRef)
    return false;

  if(pParam->eDPBMode >= AL_DPB_MAX_ENUM)
    return false;

  if(pParam->eFbStorageMode >= AL_FB_MAX_ENUM)
    return false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_PreInit(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx)
    return false;

  pCtx->PreInitMutex = Rtos_CreateMutex();

  if(!pCtx->PreInitMutex)
    return false;

  pCtx->bBasicInit = false;
  pCtx->FrmBufPool.isDecommited = false;

  return true;
}

/*****************************************************************************/
bool AL_PictMngr_BasicInit(AL_TPictMngrCtx* pCtx, AL_TPictMngrParam const* pParam)
{
  if(!pCtx)
    return false;

  if(!CheckPictMngrInitParameter(pParam))
    return false;

  if(!sMvBufPool_Init(&pCtx->MvBufPool, pParam->iNumMV))
    return false;

  AL_TDpbCallback tCallbacks =
  {
    sPictMngr_IncrementFrmBuf,
    sPictMngr_DecrementFrmBuf,
    sPictMngr_OutputFrmBuf,
    sPictMngr_IncrementMvBuf,
    sPictMngr_DecrementMvBuf,
    pCtx,
  };

  if(!AL_Dpb_Init(&pCtx->DPB, pParam->iNumDPBRef, pParam->eDPBMode, tCallbacks))
    return false;

  pCtx->eFbStorageMode = pParam->eFbStorageMode;
  pCtx->iSizeMV = pParam->iSizeMV;
  pCtx->iCurFramePOC = 0;

  pCtx->iPrevPocLSB = 0;
  pCtx->iPrevPocMSB = 0;

  pCtx->uFrameID = UndefID;
  pCtx->uMvID = UndefID;

  pCtx->iPrevFrameNum = -1;
  Rtos_GetMutex(pCtx->PreInitMutex);
  pCtx->bBasicInit = true;
  Rtos_ReleaseMutex(pCtx->PreInitMutex);
  pCtx->bForceOutput = pParam->bForceOutput;
  pCtx->bCompleteInit = false;

  pCtx->tOutputPosition = pParam->tOutputPosition;

  return true;
}

bool AL_PictMngr_CompleteInit(AL_TPictMngrCtx* pCtx, AL_TAllocator* pAllocator, bool bEnableSecondOutput)
{
  bool bSuccess = sFrmBufPool_Init(&pCtx->FrmBufPool, pAllocator, bEnableSecondOutput);
  pCtx->bCompleteInit = bSuccess;
  return bSuccess;
}

/*****************************************************************************/

bool AL_PictMngr_IsInitComplete(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->bCompleteInit;
}

/*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bBasicInit)
    return;

  AL_Dpb_Terminate(&pCtx->DPB);

  sMvBufPool_Terminate(&pCtx->MvBufPool);

  if(pCtx->bCompleteInit)
    sFrmBufPool_Terminate(&pCtx->FrmBufPool);
}

/*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx)
{
  if(pCtx->bBasicInit)
  {
    sMvBufPool_Deinit(&pCtx->MvBufPool);
    AL_Dpb_Deinit(&pCtx->DPB);

    if(pCtx->bCompleteInit)
      sFrmBufPool_Deinit(&pCtx->FrmBufPool);
  }
  Rtos_DeleteMutex(pCtx->PreInitMutex);
}

/*****************************************************************************/
void AL_PictMngr_LockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_IncrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_IncrementMvBuf(pCtx, pRefMvID[uRef]);
  }
}

/*****************************************************************************/
void AL_PictMngr_UnlockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID)
{
  for(uint8_t uRef = 0; uRef < uNumRef; ++uRef)
  {
    sPictMngr_DecrementFrmBuf(pCtx, pRefFrameID[uRef]);
    sPictMngr_DecrementMvBuf(pCtx, pRefMvID[uRef]);
  }
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->uFrameID;
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->uMvID;
}

/*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx const* pCtx)
{
  return pCtx->iCurFramePOC;
}

/***************************************************************************/
static void ChangePictChromaMode(AL_TBuffer* pBuf, AL_EChromaMode eChromaMode)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_TPicFormat tPicFmt;
  bool const bSuccess = AL_GetPicFormat(tFourCC, &tPicFmt);
  (void)bSuccess;
  Rtos_Assert(bSuccess);

  if(eChromaMode == tPicFmt.eChromaMode)
    return;
  tPicFmt = AL_GetDecPicFormat(eChromaMode, tPicFmt.uBitDepth, tPicFmt.eStorageMode, tPicFmt.bCompressed, AL_PLANE_MODE_MAX_ENUM);
  tFourCC = AL_GetDecFourCC(tPicFmt);
  AL_PixMapBuffer_SetFourCC(pBuf, tFourCC);
}

/***************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eDecodedChromaMode)
{
  Rtos_Assert(pCtx->bCompleteInit);

  (void)eDecodedChromaMode;
  pCtx->uFrameID = sFrmBufPoolFifo_Pop(&pCtx->FrmBufPool);

  if(pCtx->uFrameID == UndefID)
    return false;

  pCtx->uMvID = sMvBufPool_GetFreeBufID(&pCtx->MvBufPool);
  Rtos_Assert(pCtx->uMvID != UndefID);

  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uFrameID);

  AL_PixMapBuffer_SetDimension(pBuffers->pFrame, tDim);

  ChangePictChromaMode(pBuffers->pFrame, eDecodedChromaMode);

  pCtx->FrmBufPool.array[pCtx->uFrameID].bStartsNewCVS = bStartsNewCVS;

  return true;
}

/*****************************************************************************/
static void sFrmBufPool_SignalCallbackReleaseIsDone(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pReleasedFrame)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  int32_t const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pFrmBufPool, pReleasedFrame);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pFrmBufPool, pReleasedFrame) == false;
  (void)bNotInFifo;
  Rtos_Assert(bNotInFifo);
  Rtos_Assert(pFrmBufPool->array[iFrameID].bWillBeOutputted || pFrmBufPool->array[iFrameID].bOutEarly);
  pFrmBufPool->array[iFrameID].bWillBeOutputted = false;
  AL_TRecBuffers tBuffers;
  Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID), sizeof(tBuffers));
  sFrmBufPool_RemoveID(pFrmBufPool, iFrameID);
  sRecBuffers_Release(&tBuffers, pFrmBufPool);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/***************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx)
{
  Rtos_Assert(pCtx->bCompleteInit);

  if(pCtx->uMvID != UndefID)
  {
    sMvBufPool_DecrementBufID(&pCtx->MvBufPool, pCtx->uMvID);
    pCtx->uMvID = UndefID;
  }

  if(pCtx->uFrameID != UndefID)
  {
    sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, pCtx->uFrameID, pCtx->bForceOutput);
    pCtx->uFrameID = UndefID;
  }
}

/*************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bBasicInit)
    return;

  AL_Dpb_Flush(&pCtx->DPB);
  AL_Dpb_BeginNewSeq(&pCtx->DPB);
}

/*************************************************************************/
void AL_PictMngr_UpdateDPBInfo(AL_TPictMngrCtx* pCtx, uint8_t uMaxRef)
{
  AL_Dpb_SetNumRef(&pCtx->DPB, uMaxRef);
}

/*****************************************************************************/
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx const* pCtx)
{
  return AL_Dpb_GetLastPicID(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int32_t iFramePOC, AL_EPicStruct ePicStruct, uint32_t uPocLsb, int32_t iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT, uint8_t uSubpicFlag)
{
  uint8_t uNode = AL_Dpb_GetNextFreeNode(&pCtx->DPB);

  if(uNode == uEndOfList)
    return;

  if(!AL_Dpb_NodeIsReset(&pCtx->DPB, uNode))
    AL_Dpb_Remove(&pCtx->DPB, uNode);

  AL_Dpb_Insert(&pCtx->DPB, iFramePOC, ePicStruct, uPocLsb, uNode, iFrameID, uMvID, pic_output_flag, eMarkingFlag, uNonExisting, eNUT, uSubpicFlag);
}

/***************************************************************************/
static void sFrmBufPool_UpdateCRC(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, uint32_t uCRC)
{
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pFrmBufPool->array[iFrameID].uCRC = uCRC;
}

/***************************************************************************/
static void sFrmBufPool_UpdateCrop(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, AL_TCropInfo const* pCrop)
{
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE && pCrop);
  pFrmBufPool->array[iFrameID].tCrop = *pCrop;
}

/***************************************************************************/
static void sFrmBufPool_UpdatePicStruct(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, AL_EPicStruct ePicStruct)
{
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pFrmBufPool->array[iFrameID].ePicStruct = ePicStruct;
}

/***************************************************************************/
static void sFrmBufPool_UpdateError(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID, AL_ERR error)
{
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);
  pFrmBufPool->array[iFrameID].eError = error;
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, int32_t iFrameID, uint32_t uCRC)
{
  sFrmBufPool_UpdateCRC(&pCtx->FrmBufPool, iFrameID, uCRC);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, int32_t iFrameID, AL_TCropInfo const* pCrop)
{
  sFrmBufPool_UpdateCrop(&pCtx->FrmBufPool, iFrameID, pCrop);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferPicStruct(AL_TPictMngrCtx* pCtx, int32_t iFrameID, AL_EPicStruct ePicStruct)
{
  sFrmBufPool_UpdatePicStruct(&pCtx->FrmBufPool, iFrameID, ePicStruct);
}

/***************************************************************************/
void AL_PictMngr_UpdateDisplayBufferError(AL_TPictMngrCtx* pCtx, int32_t iFrameID, AL_ERR eError)
{
  sFrmBufPool_UpdateError(&pCtx->FrmBufPool, iFrameID, eError);
}

/***************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, int32_t iFrameID)
{
  AL_Dpb_EndDecoding(&pCtx->DPB, iFrameID);
}

/***************************************************************************/
void AL_PictMngr_UnlockID(AL_TPictMngrCtx* pCtx, int32_t iFrameID, int32_t iMotionVectorID)
{
  Rtos_Assert(pCtx->bCompleteInit);
  sFrmBufPool_DecrementBufID(&pCtx->FrmBufPool, iFrameID, pCtx->bForceOutput);
  sMvBufPool_DecrementBufID(&pCtx->MvBufPool, iMotionVectorID);
}

/***************************************************************************/
static void sFrmBufPool_GetInfoDecode(AL_TPictMngrCtx* pCtx, int32_t iFrameID, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_EFbStorageMode eFbStorageMode, bool bDisplayInfo)
{
  Rtos_Assert(pCtx->bCompleteInit);
  Rtos_Assert(pInfo);

  AL_TFrmBufPool* pFrmBufPool = &pCtx->FrmBufPool;

  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID);
  bool const bRecNotNull = sRecBuffers_AreNotNull(pBuffers, pFrmBufPool);
  (void)bRecNotNull;
  Rtos_Assert(bRecNotNull);

  AL_TBuffer* pBuf = bDisplayInfo ? sRecBuffers_GetDisplayBuffer(pBuffers, pFrmBufPool) : pBuffers->pFrame;
  AL_TPixMapMetaData* pMetaSrc = (AL_TPixMapMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP);
  Rtos_Assert(pMetaSrc);

  pInfo->tCrop = pFrmBufPool->array[iFrameID].tCrop;
  pInfo->uBitDepthY = pInfo->uBitDepthC = AL_GetBitDepth(pMetaSrc->tFourCC);
  pInfo->uCRC = pFrmBufPool->array[iFrameID].uCRC;
  pInfo->tDim = AL_PixMapBuffer_GetDimension(pBuf);
  pInfo->eFbStorageMode = eFbStorageMode;
  pInfo->ePicStruct = pFrmBufPool->array[iFrameID].ePicStruct;
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  Rtos_Assert(tFourCC != 0);
  pInfo->eChromaMode = AL_GetChromaMode(tFourCC);
  pInfo->eOutputID = AL_OUTPUT_MAIN;
  pInfo->tPos = pCtx->tOutputPosition;

  if(pStartsNewCVS)
    *pStartsNewCVS = pFrmBufPool->array[iFrameID].bStartsNewCVS;
}

/***************************************************************************/
static AL_TBuffer* AL_PictMngr_GetDisplayBuffer2(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, int32_t iFrameID)
{
  Rtos_Assert(pCtx->bCompleteInit);

  if(iFrameID == UndefID)
    return NULL;

  AL_EFbStorageMode eOutputStorageMode = pCtx->eFbStorageMode;

  if(pInfo)
    sFrmBufPool_GetInfoDecode(pCtx, iFrameID, pInfo, pStartsNewCVS, eOutputStorageMode, true);

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);

  AL_TBuffer* displayBuffer = sRecBuffers_GetDisplayBuffer(pRecBuffers, &pCtx->FrmBufPool);

  if(pInfo)
  {
    AL_TDisplayInfoMetaData* pMeta = (AL_TDisplayInfoMetaData*)(AL_Buffer_GetMetaData(displayBuffer, AL_META_TYPE_DISPLAY_INFO));

    if(pMeta)
    {
      pMeta->ePicStruct = pInfo->ePicStruct;
      pMeta->tCrop = pInfo->tCrop;
      pMeta->uCrc = pInfo->uCRC;
      pMeta->uStreamBitDepthY = pInfo->uBitDepthY;
      pMeta->uStreamBitDepthC = pInfo->uBitDepthC;
      pMeta->eOutputID = pInfo->eOutputID;

    }
  }

  return displayBuffer;
}

/***************************************************************************/
AL_TBuffer* AL_PictMngr_ForceDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, int32_t iFrameID)
{
  Rtos_Assert(pCtx->bCompleteInit);
  Rtos_Assert(pCtx->bForceOutput);
  AL_TFrmBufPool* pFrmBufPool = &pCtx->FrmBufPool;
  Rtos_GetMutex(pFrmBufPool->Mutex);
  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];
  pFrame->bOutEarly = true;
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
  return AL_PictMngr_GetDisplayBuffer2(pCtx, pInfo, pStartsNewCVS, iFrameID);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS)
{
  if(!pCtx->bBasicInit)
    return NULL;

  return AL_PictMngr_GetDisplayBuffer2(pCtx, pInfo, pStartsNewCVS, AL_Dpb_GetDisplayBuffer(&pCtx->DPB));
}

/*************************************************************************/
static void sPictMngr_ReleaseDisplayBuffer(AL_TFrmBufPool* pFrmBufPool, int32_t iFrameID)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  Rtos_Assert(iFrameID >= 0 && iFrameID < FRM_BUF_POOL_SIZE);

  AL_TFrameFifo* pFrame = &pFrmBufPool->array[iFrameID];

  if(pFrame->bOutEarly)
  {
    pFrame->bOutEarly = false;

    if((pFrame->iAccessCnt == 0) && pFrame->bWillBeOutputted)
    {
      pFrame->bWillBeOutputted = false;
      AL_TRecBuffers tBuffers;
      Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID), sizeof(tBuffers));
      sFrmBufPool_RemoveID(pFrmBufPool, iFrameID);
      bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pFrmBufPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pFrmBufPool)) == false;
      (void)bNotInFifo;
      Rtos_Assert(bNotInFifo);
      sFrmBufPoolFifo_PushBack(pFrmBufPool, &tBuffers);
    }

    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return;
  }

  Rtos_Assert(pFrame->bWillBeOutputted == true);

  pFrame->bWillBeOutputted = false;

  if(pFrame->iAccessCnt == 0)
  {
    AL_TRecBuffers tBuffers;
    Rtos_Memcpy(&tBuffers, sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID), sizeof(tBuffers));
    sFrmBufPool_RemoveID(pFrmBufPool, iFrameID);
    bool const bNotInFifo = sFrmBufPoolFifo_IsInFifo(pFrmBufPool, sRecBuffers_GetDisplayBuffer(&tBuffers, pFrmBufPool)) == false;
    (void)bNotInFifo;
    Rtos_Assert(bNotInFifo);
    sFrmBufPoolFifo_PushBack(pFrmBufPool, &tBuffers);
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, int32_t iFrameID)
{
  Rtos_Assert(pCtx);

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
  return sRecBuffers_GetDisplayBuffer(pRecBuffers, &pCtx->FrmBufPool);
}

/*************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx* pCtx, int32_t iFrameID)
{
  Rtos_Assert(pCtx);
  return sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID)->pFrame;
}

/*************************************************************************/
bool AL_PictMngr_GetFrameEncodingError(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pDisplayBuf, AL_ERR* pError)
{
  if(!pCtx->bCompleteInit)
    return false;

  if(pDisplayBuf == NULL)
    return false;

  int32_t iFrameID = sFrmBufPool_GetFrameIDFromBuf(&pCtx->FrmBufPool, pDisplayBuf);

  if(pError != NULL && iFrameID != -1)
  {
    *pError = pCtx->FrmBufPool.array[iFrameID].eError;
    return true;
  }

  return false;
}

/*************************************************************************/
static bool sFrmBufPool_FillRecBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pDisplayBuf, AL_TRecBuffers* pRecBuffers)
{
  (void)pFrmBufPool;

  pRecBuffers->pFrame = pDisplayBuf;

  AL_Buffer_Ref(pDisplayBuf);

  return true;
}

/*************************************************************************/
static bool sFrmBufPool_PutDisplayBuffer(AL_TFrmBufPool* pFrmBufPool, AL_TBuffer* pBuf)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);
  bool bSucceed = false;

  Rtos_Assert(pBuf);

  int32_t const iFrameID = sFrmBufPool_GetFrameIDFromDisplay(pFrmBufPool, pBuf);

  if(iFrameID == -1)
  {
    AL_TRecBuffers tRecBuffers;

    if(sFrmBufPool_FillRecBuffer(pFrmBufPool, pBuf, &tRecBuffers))
    {
      if(sFrmBufPoolFifo_PushBack(pFrmBufPool, &tRecBuffers))
        bSucceed = true;
      else
        sRecBuffers_Release(&tRecBuffers, pFrmBufPool);
    }
  }
  else
  {
    sPictMngr_ReleaseDisplayBuffer(pFrmBufPool, iFrameID);
    bSucceed = true;
  }

  Rtos_ReleaseMutex(pFrmBufPool->Mutex);

  return bSucceed;
}

/*************************************************************************/
bool AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf)
{
  Rtos_Assert(pCtx->bCompleteInit);
  return sFrmBufPool_PutDisplayBuffer(&pCtx->FrmBufPool, pBuf);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx)
{
  Rtos_Assert(pCtx->bBasicInit);
  AL_Dpb_ReleaseDisplayBuffer(&pCtx->DPB);
}

/*****************************************************************************/
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame)
{
  Rtos_Assert(pCtx->bCompleteInit);
  sFrmBufPool_SignalCallbackReleaseIsDone(&pCtx->FrmBufPool, pReleasedFrame);
}

/*****************************************************************************/
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx)
{
  Rtos_GetMutex(pCtx->PreInitMutex);

  if(!pCtx->bBasicInit)
  {
    pCtx->FrmBufPool.isDecommited = true;
    Rtos_ReleaseMutex(pCtx->PreInitMutex);
    return;
  }
  Rtos_ReleaseMutex(pCtx->PreInitMutex);

  if(pCtx->bCompleteInit)
    sFrmBufPoolFifo_Decommit(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static AL_TBuffer* sFrmBufPoolFifo_FlushOneDisplayBuffer(AL_TFrmBufPool* pFrmBufPool)
{
  Rtos_GetMutex(pFrmBufPool->Mutex);

  int32_t iFifoHead = pFrmBufPool->iFifoHead;

  if((iFifoHead == -1) && (pFrmBufPool->iFifoTail == -1))
  {
    Rtos_ReleaseMutex(pFrmBufPool->Mutex);
    return NULL;
  }

  Rtos_Assert(iFifoHead != -1);
  AL_TFrameFifo const* pHeadBuf = &pFrmBufPool->array[iFifoHead];
  (void)pHeadBuf;
  Rtos_Assert(pHeadBuf->iAccessCnt == 0);
  Rtos_Assert((!pHeadBuf->bOutEarly && (pHeadBuf->bWillBeOutputted == false)) || pHeadBuf->bOutEarly);

  int32_t const iFrameID = RemoveBufferFromFifo(pFrmBufPool);
  AL_TFrameFifo* removedBuf = &pFrmBufPool->array[iFrameID];
  removedBuf->iAccessCnt = 0;

  if(!removedBuf->bOutEarly)
    removedBuf->bWillBeOutputted = true;
  AL_TRecBuffers* pBuffers = sFrmBufPool_GetBufferFromID(pFrmBufPool, iFrameID);
  Rtos_ReleaseMutex(pFrmBufPool->Mutex);

  return sRecBuffers_GetDisplayBuffer(pBuffers, pFrmBufPool);
}

/*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx)
{
  if(!pCtx->bCompleteInit)
    return NULL;

  return sFrmBufPoolFifo_FlushOneDisplayBuffer(&pCtx->FrmBufPool);
}

/*****************************************************************************/
static void FillPocAndLongtermLists(AL_TDpb const* pDpb, TBufferPOC* pPoc, AL_TDecSliceParam const* pSliceParam)
{
  int32_t* pPocList = (int32_t*)(pPoc->tMD.pVirtualAddr);

  if(pPocList == NULL)
    return;

  uint16_t* pLongTermList = (uint16_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_LONG_TERM_OFFSET);
  uint16_t* pAvailableRefList = (uint16_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_AVAILABLE_REF_OFFSET);
  uint32_t* pSubpicList = (uint32_t*)(pPoc->tMD.pVirtualAddr + POCBUFF_SUBPIC_OFFSET);

  if(!pSliceParam->uFirstLcuSliceSegment)
  {
    *pLongTermList = 0;
    *pAvailableRefList = 0;
    *pSubpicList = 0;

    for(int32_t i = 0; i < MAX_REF; ++i)
      pPocList[i] = UINT32_MAX;
  }

  AL_Dpb_FillList(pDpb, pPocList, pLongTermList, pAvailableRefList, pSubpicList);
}

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecSliceParam const* pSliceParam, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, AL_TRecBuffers* pRecs)
{
  Rtos_Assert(pCtx->bCompleteInit);
  (void)pListVirtAddr; // only used for traces

  if(pCtx->uFrameID == UndefID)
    return false;

  AL_TRecBuffers* pRecBuffers = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, pCtx->uFrameID);
  pRecs->pFrame = pRecBuffers->pFrame;

  if(pMV && pPOC)
  {
    if(pCtx->uMvID == UndefID)
      return false;

    pMV->tMD = pCtx->MvBufPool.pMvBufs[pCtx->uMvID].tMD;
    pPOC->tMD = pCtx->MvBufPool.pPocBufs[pCtx->uMvID].tMD;

    FillPocAndLongtermLists(&pCtx->DPB, pPOC, (AL_TDecSliceParam const*)pSliceParam);
  }

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pRecs->pFrame);
  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(tFourCC, &tPicFormat);

  AL_EPlaneId eFirstCPlane = AL_PLANE_MODE_PLANAR == tPicFormat.ePlaneMode ? AL_PLANE_U : AL_PLANE_UV;

  if(pListAddr && pListAddr->tMD.pVirtualAddr)
  {
    TRefListOffsets tRefListOffsets;
    AL_GetRefListOffsets(&tRefListOffsets, AL_CODEC_INVALID, tPicFormat, sizeof(AL_PADDR));

    AL_PADDR* pAddr = (AL_PADDR*)pListAddr->tMD.pVirtualAddr;
    AL_PADDR* pColocMvList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uColocMVOffset);
    AL_PADDR* pColocPocList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uColocPocOffset);
    AL_PADDR* pFbcList = (AL_PADDR*)(pListAddr->tMD.pVirtualAddr + tRefListOffsets.uMapOffset);

    for(int32_t i = 0; i < PIC_ID_POOL_SIZE; ++i)
    {
      uint8_t uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, i);

      if(uNodeID == UndefID)
      {
        if(pSliceParam->bValidConceal && pCtx->DPB.uCountPic)
          uNodeID = AL_Dpb_ConvertPicIDToNodeID(&pCtx->DPB, pSliceParam->uConcealPicID);
      }

      AL_TBuffer const* pRefBuf = NULL;
      TBufferMV const* pRefMV = NULL;
      TBufferPOC const* pRefPOC = NULL;

      if(uNodeID != UndefID)
      {
        int32_t iFrameID = AL_Dpb_GetFrmID_FromNode(&pCtx->DPB, uNodeID);
        uint8_t uMvID = AL_Dpb_GetMvID_FromNode(&pCtx->DPB, uNodeID);
        AL_TRecBuffers* pBufs = sFrmBufPool_GetBufferFromID(&pCtx->FrmBufPool, iFrameID);
        pRefBuf = pBufs->pFrame;
        pRefMV = &pCtx->MvBufPool.pMvBufs[uMvID];
        pRefPOC = &pCtx->MvBufPool.pPocBufs[uMvID];
      }
      else
      {
        // Use current Rec & MV Buffers to avoid non existing ref/coloc
        pRefBuf = pRecs->pFrame;
        pRefMV = pMV;
        pRefPOC = pPOC;
      }

      pAddr[i] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, AL_PLANE_Y);
      pAddr[PIC_ID_POOL_SIZE + i] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, eFirstCPlane);
      pAddr[i] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, pCtx->tOutputPosition, AL_PLANE_Y);
      pAddr[PIC_ID_POOL_SIZE + i] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, pCtx->tOutputPosition, eFirstCPlane);

      pFbcList[i] = 0;
      pFbcList[PIC_ID_POOL_SIZE + i] = 0;

      pColocMvList[i] = pRefMV->tMD.uPhysicalAddr;
      pColocPocList[i] = pRefPOC->tMD.uPhysicalAddr;

    }
  }
  return true;
}

/*!@}*/
