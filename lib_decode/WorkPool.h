// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferAPI.h"

typedef struct
{
  AL_TBuffer* buf;
  int32_t prev;
  int32_t next;
}WorkPoolElem;

typedef struct
{
  WorkPoolElem* elems;
  AL_MUTEX lock;
  AL_EVENT spaceAvailable;
  int32_t freeHead;
  int32_t freeQueue;
  int32_t filledHead;
  int32_t filledQueue;
  int32_t capacity;
}WorkPool;

bool AL_WorkPool_Init(WorkPool* pool, int32_t iMaxBufNum);
void AL_WorkPool_Deinit(WorkPool* pool);
void AL_WorkPool_Remove(WorkPool* pool, AL_TBuffer* pBuf);
void AL_WorkPool_PushBack(WorkPool* pool, AL_TBuffer* pBuf);
bool AL_WorkPool_IsEmpty(WorkPool* pool); /* Not thread safe */
bool AL_WorkPool_IsFull(WorkPool* pool);  /* Not thread safe */
int32_t AL_WorkPool_GetSize(WorkPool* pool);  /* Not thread safe */
