// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

/*****************************************************************************/
#define AV1_LF_REF_DELTA_IDX 8
#define AV1_LF_NUM_LEVEL 4
#define AV1_LF_NUM_MODE_DELTA_FRAME 2
#define AV1_MFMV_STACK_SIZE 3

#define AV1_REF_SCALE_SHIFT 14

/*****************************************************************************/
#define AV1_ICDF_PROB_SIZE 2
#define AV1_ICDF_PROB_PER_WORD 4
#define AV1_ICDF_COEFF_PROBS_WORDS 1024
#define AV1_ICDF_MODE_PROBS_WORDS 1280
#define AV1_ICDF_COEFF_PROBS_SIZE (AV1_ICDF_COEFF_PROBS_WORDS * AV1_ICDF_PROB_PER_WORD * AV1_ICDF_PROB_SIZE)
#define AV1_ICDF_MODE_PROBS_SIZE (AV1_ICDF_MODE_PROBS_WORDS * AV1_ICDF_PROB_PER_WORD * AV1_ICDF_PROB_SIZE)

/*****************************************************************************/
typedef enum
{
  AL_AV1_SINGLE_REFERENCE,
  AL_AV1_COMPOUND_REFERENCE,
  AL_AV1_REFERENCE_MODE_SELECT,
  AL_AV1_REFERENCE_MODES,
}AL_EAv1ReferenceMode;
