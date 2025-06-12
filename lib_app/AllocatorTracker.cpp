// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

extern "C"
{
#include "lib_common/Allocator.h"
#include "lib_common/AllocatorTracker.h"
#include "lib_rtos/types.h"
}
#include <iostream>
#include <iomanip>
#include <string>
#include <map>

using namespace std;

typedef map<size_t, uint16_t> FrequenciesPerSizes;

struct AllocatorTracker
{
  AL_TAllocatorVTable const* vtable;
  AL_TAllocator* realAllocator;
  map<string, FrequenciesPerSizes> allocsPerName;
  uint64_t uTotalSize = 0;
  AL_ETrackDmaMode eTrackDmaMode = AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE;
};

static inline int32_t bytes_to_megabytes(AL_64U bytes)
{
  return bytes >> 20;
}

static void dma_usage(AllocatorTracker const* tracker)
{
  cout << endl << "DMA USAGE:" << endl;
  cout << "Total Dma Used : " << tracker->uTotalSize << " bytes, " << bytes_to_megabytes(tracker->uTotalSize) << "MB" << endl;

  for(auto const& alloc : tracker->allocsPerName)
  {
    auto const& name = alloc.first;
    auto const& frequenciesPerSizes = alloc.second;
    AL_64U uCurrentNameTotalSize = 0;

    cout << setfill(' ') << "-> " << setw(24) << left << name;

    for(auto const& freqSize : frequenciesPerSizes)
    {
      auto const size = freqSize.first;
      auto const freq = freqSize.second;
      cout << freq << " * " << size << ", ";
      uCurrentNameTotalSize += freq * (size_t)size;
    }

    cout << "(total: ~" << bytes_to_megabytes(uCurrentNameTotalSize) << "MB" << ")" << endl;
  }

  cout.flush();
}

static void destroy(AL_TAllocator* handle)
{
  auto self = (AllocatorTracker*)handle;
  AL_Allocator_Destroy(self->realAllocator);

  if((self->eTrackDmaMode == AL_ETrackDmaMode::AL_TRACK_DMA_MODE_LIVE) || (self->eTrackDmaMode == AL_ETrackDmaMode::AL_TRACK_DMA_MODE_SUMMARY))
  {
    dma_usage(self);
  }
  delete self;
}

static AL_HANDLE allocNamed(AL_TAllocator* handle, size_t size, char const* name)
{
  auto self = (AllocatorTracker*)handle;
  self->uTotalSize += size;
  self->allocsPerName[string(name)][size]++;

  if(self->eTrackDmaMode == AL_ETrackDmaMode::AL_TRACK_DMA_MODE_LIVE)
  {
    dma_usage(self);
  }
  return AL_Allocator_Alloc(self->realAllocator, size);
}

static AL_HANDLE alloc(AL_TAllocator* handle, size_t size)
{
  return allocNamed(handle, size, "unknown");
}

static bool free(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_Free(self->realAllocator, buf);
}

static AL_VADDR getVirtualAddr(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_GetVirtualAddr(self->realAllocator, buf);
}

static AL_PADDR getPhysicalAddr(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_GetPhysicalAddr(self->realAllocator, buf);
}

static void syncForCpu(AL_TAllocator* handle, AL_VADDR pVirtualAddr, size_t zSize)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_SyncForCpu(self->realAllocator, pVirtualAddr, zSize);
}

static void syncForDevice(AL_TAllocator* handle, AL_VADDR pVirtualAddr, size_t zSize)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_SyncForDevice(self->realAllocator, pVirtualAddr, zSize);
}

AL_TAllocatorVTable constexpr trackerVtable =
{
  destroy,
  alloc,
  free,
  getVirtualAddr,
  getPhysicalAddr,
  allocNamed,
  syncForCpu,
  syncForDevice,
};

AL_TAllocator* createAllocatorTracker(AL_TAllocator* pAllocator, AL_ETrackDmaMode eTrackDmaMode)
{
  switch(eTrackDmaMode)
  {
  case AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE:
  default:
  {
    return pAllocator;
    break;
  }
  case AL_ETrackDmaMode::AL_TRACK_DMA_MODE_LIVE:
  case AL_ETrackDmaMode::AL_TRACK_DMA_MODE_SUMMARY:
  {
    auto tracker = new AllocatorTracker;
    tracker->vtable = &trackerVtable;
    tracker->realAllocator = pAllocator;
    tracker->eTrackDmaMode = eTrackDmaMode;
    return (AL_TAllocator*)tracker;
  }
  }
}
