// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_common
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/common_syntax_elements.h"
#include "lib_common/ScalingList.h"

static const size_t AL_SL_INTRA = 0;
static const size_t AL_SL_INTER = 1;
typedef uint32_t AL_TLevels4x4[4 * 4];
typedef uint32_t AL_TLevels8x8[8 * 8];
typedef uint32_t AL_TLevelsDC[4];

/*****************************************************************************
   \brief Scaling List Matrices in hardware preprocessed format
*****************************************************************************/
typedef struct AL_THwScalingList
{
  AL_TLevels8x8 t32x32;
  AL_TLevels8x8 t16x16Y;
  AL_TLevels8x8 t16x16Cb;
  AL_TLevels8x8 t16x16Cr;
  AL_TLevels8x8 t8x8Y;
  AL_TLevels8x8 t8x8Cb;
  AL_TLevels8x8 t8x8Cr;
  AL_TLevels4x4 t4x4Y;
  AL_TLevels4x4 t4x4Cb;
  AL_TLevels4x4 t4x4Cr;
  AL_TLevelsDC tDC;
}AL_THwScalingList;

/*****************************************************************************
   \brief Converts AVC Scaling List matrices from software user-friendly format to
   Hardware encoder preprocessed format.
   \param[in]  pSclLst pointer to Scaling List in Software format
   \param[in]  chroma_format_idc chroma mode id
   \param[out] pHwSclLst pointer to Hardware formatted Scaling list that receives
   the preprocessed matrices
*****************************************************************************/
void AL_AVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, uint8_t chroma_format_idc, AL_THwScalingList(*pHwSclLst)[2][6]);

/*****************************************************************************
   \brief Converts HEVC Scaling List matrices from software user-friendly format to
   Hardware encoder preprocessed format.
   \param[in]  pSclLst pointer to Scaling List in Software format
   \param[out] pHwSclLst pointer to Hardware formatted Scaling list that receives
   the preprocessed matrices
*****************************************************************************/
void AL_HEVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList(*pHwSclLst)[2][6]);

/*!@}*/
