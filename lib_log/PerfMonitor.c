// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "PerfMonitor.h"
#include "lib_rtos/lib_rtos.h"

void AL_PerformanceLog(AL_PerfPrintCtx* pPrintCtx)
{
  Rtos_Log(0, "\nPERFORMANCE %s %d %d %d %d %c\n", pPrintCtx->sProcess, pPrintCtx->uCoreID, pPrintCtx->uFrameNum, pPrintCtx->uCoreCycle, pPrintCtx->uNumBytes, pPrintCtx->cFrameType);
}
