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
}AL_TDecSchedulerCore;

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
  AL_64U uVersion; /*!< Version */
}AL_TDecSchedulerVersion;

/****************************************************************************/
typedef enum
{
  AL_DEC_SCHEDULER_VERSION, /*!< reference: AL_TDecSchedulerVersion */
  AL_DEC_SCHEDULER_CORE, /*!< reference: AL_TDecSchedulerCore */
  AL_DEC_SCHEDULER_CHANNEL_TRACE_CALLBACK, /*!< reference: AL_TDecSchedulerChannelTraceCallback */
  AL_DEC_SCHEDULER_MAX_ENUM,
}AL_EDecSchedulerInfo;
