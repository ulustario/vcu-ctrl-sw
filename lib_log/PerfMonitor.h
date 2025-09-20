// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_log/LoggerInterface.h"

static uint8_t const PERF_VAL_X = 255;
static uint8_t const PERF_VAL_Z = 254;

typedef struct
{
  AL_ILogger* pLogger;
  bool bEnabled;
}AL_TLoggerCtx;

#define EventLogV(LoggerCtx, Signal, Value)
#define EventLog(LoggerCtx, Signal)

typedef struct
{
  const char* sProcess;
  uint8_t uCoreID;
  uint32_t uFrameNum;
  uint32_t uCoreCycle;
  uint32_t uNumBytes;
  char cFrameType;
}AL_PerfPrintCtx;

void AL_PerformanceLog(AL_PerfPrintCtx* pPrintCtx);
