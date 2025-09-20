// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/AomDefinesInternal.h"
#include "lib_common/Av1Defines.h"
#include "lib_common_dec/Av1Defines.h"

/*****************************************************************************/
typedef struct
{
  bool FeatureEnabled[AV1_SEG_LVL_MAX];
  int16_t FeatureData[AV1_SEG_LVL_MAX];
}AL_TAv1SegmentationParams;

/*****************************************************************************/
typedef struct
{
  int8_t loop_filter_mode_deltas[AV1_LF_NUM_MODE_DELTA_FRAME];
  int8_t loop_filter_ref_deltas[AOM_TOTAL_REFS_PER_FRAME];
}AL_TAv1LoopFilterParams;

/*****************************************************************************/
typedef struct
{
  int32_t vValues[AV1_NUM_GLOBAL_MOTION_PARAMS];
}AL_TAv1GmParams;

/*****************************************************************************/
typedef struct
{
  uint8_t apply_grain;
  uint16_t grain_seed;
  uint8_t num_y_points;
  uint8_t chroma_scaling_from_luma;
  uint8_t num_cb_points;
  uint8_t num_cr_points;
  uint8_t grain_scaling_minus_8;
  uint8_t ar_coeff_lag;
  uint8_t ar_coeffs_y_plus_128[24];
  uint8_t ar_coeffs_cb_plus_128[25];
  uint8_t ar_coeffs_cr_plus_128[25];
  uint8_t ar_coeff_shift_minus_6;
  uint8_t grain_scale_shift;
  uint8_t cb_mult;
  uint8_t cb_luma_mult;
  uint16_t cb_offset;
  uint8_t cr_mult;
  uint8_t cr_luma_mult;
  uint16_t cr_offset;
  uint8_t overlap_flag;
  uint8_t clip_to_restricted_range;
  uint8_t scaling_lut[AV1_NUM_PLANES][256];
}AL_TAv1FilmGrainParams;

/*****************************************************************************/
typedef struct
{
  AL_TAv1GmParams vGmParams[AV1_REFS_PER_FRAME];
  AL_TAv1SegmentationParams vSegmentParams[AOM_MAX_SEGMENTS];
  AL_TAv1FilmGrainParams vFilmGrainParams;
  AL_TAv1LoopFilterParams tLoopFilterParams;
}AL_TAv1FrameContext;
