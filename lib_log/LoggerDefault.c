// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <string.h>
#include "lib_log/LoggerDefault.h"
#include "lib_rtos/lib_rtos.h"

typedef struct AL_TDefaultLogger
{
  AL_TLoggerVTable const* vtable;
  AL_HANDLE handle;
  AL_TAllocator* allocator;
  AL_ITimer* timer;
  AL_TDefaultLoggerEvents* events;
  AL_MUTEX mutex;
}AL_TDefaultLogger;

static void store(AL_TDefaultLogger* logger, const char* label, AL_64U timestamp)
{
  int32_t count = logger->events->count;
  strcpy(logger->events->samples[count].label, label);
  logger->events->samples[count].timestamp = timestamp;
  logger->events->count++;
}

static bool hasLooped(AL_TDefaultLogger* logger, AL_64U time)
{
  int32_t count = logger->events->count;
  return (count > 0) && (logger->events->samples[count - 1].timestamp > time);
}

static void defaultLog(AL_ILogger* logger, char const label[MAX_LOG_LABEL_SIZE])
{
  AL_TDefaultLogger* l = (AL_TDefaultLogger*)logger;

  Rtos_GetMutex(l->mutex);

  if(l->events->count >= l->events->max)
  {
    Rtos_ReleaseMutex(l->mutex);
    return;
  }

  AL_64U time = AL_ITimer_GetTime(l->timer);

  while(hasLooped(l, time))
    time += UINT32_MAX;

  store(l, label, time);

  Rtos_ReleaseMutex(l->mutex);
}

static void defaultDeinit(AL_ILogger* logger)
{
  if(NULL == logger)
    return;

  Rtos_DeleteMutex(((AL_TDefaultLogger*)logger)->mutex);
  AL_Allocator_Free(((AL_TDefaultLogger*)logger)->allocator, ((AL_TDefaultLogger*)logger)->handle);
}

static AL_TLoggerVTable const DefaultLoggerVtable =
{
  &defaultLog,
  &defaultDeinit,
};

AL_ILogger* AL_DefaultLogger_Init(AL_TAllocator* allocator, AL_ITimer* timer, AL_TDefaultLoggerEvents* events)
{
  if(NULL == allocator)
    return NULL;

  if(NULL == timer)
    return NULL;

  if(NULL == events)
    return NULL;

  if(events->max < 0)
    return NULL;

  if((events->max > 0) && (NULL == events->samples))
    return NULL;

  AL_HANDLE handle = AL_Allocator_AllocNamed(allocator, sizeof(AL_TDefaultLogger), "default logger");

  if(handle == NULL)
    return NULL;

  AL_TDefaultLogger* logger = (AL_TDefaultLogger*)AL_Allocator_GetVirtualAddr(allocator, handle);

  logger->mutex = Rtos_CreateMutex();

  if(NULL == logger->mutex)
  {
    AL_Allocator_Free(allocator, handle);
    return NULL;
  }

  logger->vtable = &DefaultLoggerVtable;
  logger->handle = handle;
  logger->allocator = allocator;
  logger->timer = timer;
  logger->events = events;
  logger->events->count = 0;

  AL_Allocator_SyncForDevice(allocator, (AL_VADDR)logger, sizeof(*logger));

  return (AL_ILogger*)logger;
}
