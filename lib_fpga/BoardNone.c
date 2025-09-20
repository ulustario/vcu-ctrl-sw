// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_fpga/Board.h"
#include "lib_common/Allocator.h"
#include "lib_rtos/lib_rtos.h"

AL_TIpCtrl* AL_Board_Create(char const* deviceFile)
{
  (void)deviceFile;
  Rtos_Log(AL_LOG_ERROR, "No support for board on this platform\n");
  return NULL;
}

AL_TAllocator* AL_DmaAlloc_Create(char const* deviceFile)
{
  (void)deviceFile;
  Rtos_Log(AL_LOG_ERROR, "No support for DMA on this platform\n");
  return NULL;
}
