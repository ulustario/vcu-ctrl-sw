// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_log/LoggerRtos.h"
#include "lib_rtos/lib_rtos.h"

typedef struct
{
  AL_TLoggerVTable const* vtable;
  AL_HANDLE handle;
  AL_TAllocator* allocator;
  AL_ITimer* timer;
  AL_MUTEX mutex;
}AL_TRtosLogger;

static void rtosLog(AL_ILogger* logger, char const label[MAX_LOG_LABEL_SIZE])
{
  AL_TRtosLogger* l = (AL_TRtosLogger*)logger;
  Rtos_GetMutex(l->mutex);
  Rtos_LogWithoutLevel("%u ", label[0]);
  Rtos_LogWithoutLevel("%u ", (unsigned char)label[1]);
  Rtos_LogWithoutLevel("%llu\n", AL_ITimer_GetTime(l->timer));
  Rtos_ReleaseMutex(l->mutex);
}

static void rtosDeinit(AL_ILogger* logger)
{
  if(NULL == logger)
    return;

  Rtos_DeleteMutex(((AL_TRtosLogger*)logger)->mutex);
  AL_Allocator_Free(((AL_TRtosLogger*)logger)->allocator, ((AL_TRtosLogger*)logger)->handle);
}

static AL_TLoggerVTable const RtosLoggerVtable =
{
  &rtosLog,
  &rtosDeinit,
};

AL_ILogger* AL_RtosLogger_Init(AL_TAllocator* allocator, AL_ITimer* timer)
{
  if(NULL == allocator)
    return NULL;

  if(NULL == timer)
    return NULL;

  AL_HANDLE handle = AL_Allocator_AllocNamed(allocator, sizeof(AL_TRtosLogger), "rtos logger");

  if(handle == NULL)
    return NULL;

  AL_TRtosLogger* logger = (AL_TRtosLogger*)AL_Allocator_GetVirtualAddr(allocator, handle);

  logger->mutex = Rtos_CreateMutex();

  if(NULL == logger->mutex)
  {
    AL_Allocator_Free(allocator, handle);
    return NULL;
  }

  logger->vtable = &RtosLoggerVtable;
  logger->handle = handle;
  logger->allocator = allocator;
  logger->timer = timer;

  AL_Allocator_SyncForDevice(allocator, (AL_VADDR)logger, sizeof(*logger));

  return (AL_ILogger*)logger;
}
