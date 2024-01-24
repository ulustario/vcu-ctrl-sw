// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"
#include "lib_log/TimerInterface.h"

typedef struct
{
  AL_64U timestamp;
  char label[32];
}LogEvent;

typedef struct
{
  AL_ITimer const* timer;
  LogEvent* events;
  int count;
  int maxCount;
  AL_MUTEX mutex;
}AL_Logger;

void AL_LoggerInit(AL_Logger* logger, AL_ITimer const* timer, LogEvent* buffer, int maxCount);
void AL_LoggerDeinit(AL_Logger* logger);
void AL_Log(AL_Logger* logger, char const* label);

extern AL_Logger g_Logger;
