// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_app/utils.h"

#include <string>

extern "C"
{
#include "lib_fpga/DmaAlloc.h"
#include "lib_log/LoggerDefault.h"
#include "lib_common_dec/DecoderTraceHook.h"
#include "lib_common/AllocatorTracker.h"
}

/*****************************************************************************/
typedef struct AL_TAllocator AL_TAllocator;
typedef struct AL_TIpCtrl AL_TIpCtrl;
typedef struct AL_TDriver AL_TDriver;

/*****************************************************************************/
struct CIpDeviceParam
{
  AL_EDeviceType eDeviceType;
  AL_ESchedulerType eSchedulerType;
  AL_ETrackDmaMode eTrackDmaMode = AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE;
  uint8_t uNumCore = 0;
  AL_EIpCtrlMode ipCtrlMode;
  std::string apbFile;
  bool bSelectDeviceWithLowestAvailableResources;
};

struct I_IpDevice
{
  virtual ~I_IpDevice() = default;
  virtual void* GetScheduler() = 0;
  virtual AL_TAllocator* GetAllocator() = 0;
  virtual AL_ITimer* GetTimer() = 0;
};
