// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_log/LoggerInterface.h"
#include "lib_log/TimerInterface.h"
#include "lib_common/Allocator.h"
#include "lib_rtos/lib_rtos.h"

typedef struct AL_TDefaultLoggerSample
{
  AL_64U timestamp;
  char label[MAX_LOG_LABEL_SIZE];
}AL_TDefaultLoggerSample;

typedef struct AL_TDefaultLoggerEvents
{
  AL_TDefaultLoggerSample* samples;
  int count;
  int max;
}AL_TDefaultLoggerEvents;

AL_ILogger* AL_DefaultLogger_Init(AL_TAllocator* allocator, AL_ITimer* timer, AL_TDefaultLoggerEvents* events);
