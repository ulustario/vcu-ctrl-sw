// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"

typedef struct
{
  uint64_t timestamp;
  char label[32];
}LogEvent;

typedef struct AL_t_Timer AL_Timer;
typedef struct
{
  uint32_t (* pfnGetTime)(AL_Timer const* timer);
}AL_TimerVtable;

struct AL_t_Timer
{
  const AL_TimerVtable* vtable;
};

typedef struct
{
  const AL_TimerVtable* vtable;
}AL_CpuTimer;
AL_Timer* AL_CpuTimerInit(AL_CpuTimer* timer);

typedef struct
{
  AL_Timer const* timer;
  LogEvent* events;
  int count;
  int maxCount;
  AL_MUTEX mutex;
}AL_Logger;

static inline uint32_t AL_Timer_GetTime(AL_Timer const* timer)
{
  return timer->vtable->pfnGetTime(timer);
}

void AL_LoggerInit(AL_Logger* logger, AL_Timer const* timer, LogEvent* buffer, int maxCount);
void AL_LoggerDeinit(AL_Logger* logger);
void AL_Log(AL_Logger* logger, const char* label);

extern AL_CpuTimer g_CpuTimer;
extern AL_Logger g_Logger;
