// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_log/LoggerInterface.h"
#include "lib_log/TimerInterface.h"
#include "lib_rtos/lib_rtos.h"

typedef struct
{
  AL_64U timestamp;
  char label[MAX_LOG_LABEL_SIZE];
}LogEvent;

typedef struct
{
  AL_ILoggerVtable const* vtable;
  AL_ITimer* timer;
  LogEvent* events;
  int count;
  int maxCount;
  AL_MUTEX mutex;
}AL_TDefaultLogger;

AL_ILogger* AL_DefaultLogger_Init(AL_TDefaultLogger* logger, AL_ITimer* timer, LogEvent* events, int maxCount);

AL_TDefaultLogger* AL_DefaultLogger_Get(void);
