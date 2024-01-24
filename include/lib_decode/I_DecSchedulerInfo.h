// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Core information structure
*****************************************************************************/
typedef struct
{
  int32_t iCoreFrequency;
  int32_t iMaxVideoResourcePerCore;
  int32_t iVideoResource[AL_DEC_NUM_CORES];
}AL_TIDecSchedulerCore;

/*************************************************************************//*!
   \brief Version (SemVer) structure
*****************************************************************************/
typedef union
{
  struct
  {
    uint16_t uMajor; /*!< Major version */
    uint16_t uMinor; /*!< Minor version */
    uint32_t uPatch; /*!< Patch version */
  }version;
  uint64_t uVersion; /*!< Version */
}AL_TIDecSchedulerVersion;

/****************************************************************************/
typedef enum
{
  AL_IDECSCHEDULER_VERSION, /*!< reference: AL_TIDecSchedulerVersion */
  AL_IDECSCHEDULER_CORE, /*!< reference: AL_TIDecSchedulerCore */
  AL_IDECSCHEDULER_CHANNEL_TRACE_CALLBACK, /*!< reference: AL_TIDecSchedulerChannelTraceCallback */
  AL_IDECSCHEDULER_MAX_ENUM,
}AL_EIDecSchedulerInfo;

static inline char const* ToStringIDecSchedulerInfo(AL_EIDecSchedulerInfo eInfo)
{
  switch(eInfo)
  {
  case AL_IDECSCHEDULER_CORE: return "AL_IDECSCHEDULER_CORE";
  case AL_IDECSCHEDULER_CHANNEL_TRACE_CALLBACK: return "AL_IDECSCHEDULER_CHANNEL_TRACE_CALLBACK";
  case AL_IDECSCHEDULER_VERSION: return "AL_IDECSCHEDULER_VERSION";
  case AL_IDECSCHEDULER_MAX_ENUM: return "AL_IDECSCHEDULER_MAX_ENUM";

  default: return "Unknown info";
  }

  return "Unknown info";
}
