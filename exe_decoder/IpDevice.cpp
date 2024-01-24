// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IpDevice.h"

#include <stdexcept>
#include <memory>
#include <set>
#include <cassert>

#include "IpDevice.h"
#include "IpDeviceCommon.h"
#include "lib_app/console.h"
#include "lib_app/utils.h"
#include "lib_common/Allocator.h"

extern "C"
{
#include "lib_fpga/DmaAlloc.h"
#include "lib_log/Logger.h"
#include "lib_log/TimerSoftware.h"
}

using namespace std;

AL_TAllocator* createDmaAllocator(const char* deviceName)
{
  auto h = AL_DmaAlloc_Create(deviceName);

  if(h == nullptr)
    throw runtime_error("Can't find dma allocator (trying to use " + string(deviceName) + ")");
  return h;
}

extern "C"
{
#include "lib_decode/DecSchedulerMcu.h"
}

AL_TAllocator* CreateProxyAllocator(char const*)
{
  // support for the proxy allocator isn't compiled in.
  return nullptr;
}

void CIpDevice::ConfigureMcu(AL_TDriver* driver, bool useProxy)
{
  if(useProxy)
    m_pAllocator = CreateProxyAllocator(this->m_tSelectedDevice.c_str());
  else
    m_pAllocator = createDmaAllocator(this->m_tSelectedDevice.c_str());

  if(!m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  m_pScheduler = AL_DecSchedulerMcu_Create(driver, this->m_tSelectedDevice.c_str());

  if(!m_pScheduler)
    throw runtime_error("Failed to create MCU scheduler");
}

CIpDevice::~CIpDevice()
{
  if(m_pScheduler)
    AL_IDecScheduler_Destroy(m_pScheduler);

  if(m_pAllocator)
    AL_Allocator_Destroy(m_pAllocator);
}

static std::string SelectMcuDevice(std::set<std::string> const& tDevices, bool bSelectDeviceWithLowestAvailableResources)
{
  std::string best_device;
  int32_t selected_resources = bSelectDeviceWithLowestAvailableResources ? INT32_MAX : -1;

  for(auto const& device : tDevices)
  {
    AL_IDecScheduler* scheduler = AL_DecSchedulerMcu_Create(AL_GetHardwareDriver(), device.c_str());

    if(scheduler == nullptr)
      throw runtime_error(string("Can't create MCU Scheduler: ") + device);

    int total_resources = 0;
    AL_TIDecSchedulerCore tCore;
    AL_IDecScheduler_Get(scheduler, AL_IDECSCHEDULER_CORE, &tCore);

    for(int iCore = 0; iCore < AL_DEC_NUM_CORES; iCore++)
      total_resources += tCore.iVideoResource[iCore];

    if(!bSelectDeviceWithLowestAvailableResources)
    {
      if(total_resources >= selected_resources)
      {
        selected_resources = total_resources;
        best_device = device;
      }
    }
    else
    {
      if(total_resources <= selected_resources)
      {
        selected_resources = total_resources;
        best_device = device;
      }
    }

    AL_IDecScheduler_Destroy(scheduler);
  }

  if(best_device.empty())
    throw runtime_error("Something wrong happened!");

  return best_device;
}

CIpDevice::CIpDevice(CIpDeviceParam const& param, AL_EDeviceType eDeviceType, std::set<std::string> tDevices) :
  m_tDevices(tDevices)
{
  this->m_eDeviceType = eDeviceType;

  if(param.iSchedulerType == AL_SCHEDULER_TYPE_MCU)
  {
    this->m_tSelectedDevice = SelectMcuDevice(m_tDevices, param.bSelectDeviceWithLowestAvailableResources);
    ConfigureMcu(AL_GetHardwareDriver(), false);
    return;
  }

  throw runtime_error("No support for this scheduling type");
}

AL_EDeviceType CIpDevice::GetDeviceType()
{
  return this->m_eDeviceType;
}
