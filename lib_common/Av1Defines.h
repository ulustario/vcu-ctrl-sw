// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>

#define AV1_REFS_PER_FRAME 7

// Bits of precision used for the model
#define AV1_WARPEDMODEL_PREC_BITS 16
#define AV1_GM_ABS_ALPHA_BITS 12
#define AV1_GM_ALPHA_PREC_BITS 15
#define AV1_GM_ABS_TRANS_ONLY_BITS 9
#define AV1_GM_TRANS_ONLY_PREC_BITS 3
#define AV1_GM_ABS_TRANS_BITS 12
#define AV1_GM_TRANS_PREC_BITS 6
#define AV1_NUM_GLOBAL_MOTION_PARAMS 6
#define AV1_NUM_PLANES 3
#define AV1_FILMGRAIN_MAX_POINTS 14
#define AV1_RESTORATION_TILESIZE_MAX 256

/*****************************************************************************
   \brief AV1 Motion Transformation Type
*****************************************************************************/
typedef enum
{
  AV1_IDENTITY,
  AV1_TRANSLATION,
  AV1_ROTZOOM,
  AV1_AFFINE,
  AV1_TRANS_TYPES
}EAv1MotionTransformationType;
typedef enum
{
  AV1_SEG_LVL_ALT_Q = 0, // Use alternate Quantizer ....
  AV1_SEG_LVL_ALT_LF = 1, // Use alternate loop filter value...
  AV1_SEG_LVL_REF_FRAME = 5, // Optional Segment reference frame
  AV1_SEG_LVL_SKIP = 6, // Optional Segment (0,0) + skip mode
  AV1_SEG_LVL_GLOBAL_MV = 7,
  AV1_SEG_LVL_MAX = 8 // Number of features supported
}AV1_SEG_LVL_FEATURES;

typedef enum
{
  AV1_RESTORE_NONE,
  AV1_RESTORE_WIENER,
  AV1_RESTORE_SGRPROJ,
  AV1_RESTORE_SWITCHABLE,
  AV1_RESTORE_SWITCHABLE_TYPES = AV1_RESTORE_SWITCHABLE,
  AV1_RESTORE_TYPES = 4,
}EAv1RestorationType;

static const uint8_t AV1_SEG_FEATURE_DATA_SIGNED[AV1_SEG_LVL_MAX] =
{
  1, 1, 1, 1, 1, 0, 0, 0
};

static const uint8_t AV1_SEG_FEATURE_DATA_NUMBITS[AV1_SEG_LVL_MAX] =
{
  8, 6, 6, 6, 6, 3, 0, 0
};

static const uint8_t AV1_SEG_FEATURE_DATA_MAX[AV1_SEG_LVL_MAX] =
{
  255, 63, 63, 63, 63, 7, 0, 0
};

static const EAv1RestorationType AV1_REMAP_LR_TYPE[4] =
{
  AV1_RESTORE_NONE, AV1_RESTORE_SWITCHABLE, AV1_RESTORE_WIENER, AV1_RESTORE_SGRPROJ
};
