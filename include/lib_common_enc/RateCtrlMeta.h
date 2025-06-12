// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Buffers
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common_enc/RateCtrlStats.h"

/*****************************************************************************
   \brief Configura which Rate Control Statistics are to be used
*****************************************************************************/
typedef enum AL_ERateCtrlStatMode
{
  AL_RATECTRL_STAT_MODE_NONE = 0x0,
  AL_RATECTRL_STAT_MODE_DEFAULT = 0x01,
  AL_RATECTRL_STAT_MODE_MV = 0x02,
  AL_RATECTRL_STAT_MODE_QUALITY = 0x04,
  AL_RATECTRL_STAT_MODE_MAX_ENUM,
}AL_ERateCtrlStatMode;

/*****************************************************************************
   \brief MetaData gathering encode-statistics useful for rate-control
   algorithms
*****************************************************************************/
typedef struct AL_TRateCtrlMetaData
{
  AL_TMetaData tMeta;
  AL_ERateCtrlStatMode eStatCtrl;
  bool bFilled;
  AL_TRateCtrl_Statistics tRateCtrlStats;
  AL_TBuffer* pMVBuf;
}AL_TRateCtrlMetaData;

/*****************************************************************************
   \brief Create a RateCtrl metadata.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_TRateCtrlMetaData* AL_RateCtrlMetaData_CustomCreate(AL_TAllocator* pAllocator, AL_ERateCtrlStatMode eStatCtrl, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec);

/*****************************************************************************
   \brief Create a RateCtrl metadata.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_DEPRECATED("")
AL_TRateCtrlMetaData * AL_RateCtrlMetaData_Create(AL_TAllocator * pAllocator, AL_TDimension tDim, uint8_t uLog2MaxCuSize, AL_ECodec eCodec);

/*****************************************************************************
   \brief Create a RateCtrl metadata with buffer for motion vector. Caller must
   be sure pMVBuf size is correct.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create_WithBuffer(AL_TBuffer* pMVBuf);

/*!@}*/
