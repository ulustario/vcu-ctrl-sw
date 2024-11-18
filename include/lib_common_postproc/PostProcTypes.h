// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

/*****************************************************************************
   \brief Post-processing modes
*****************************************************************************/

typedef enum AL_EPostProcRotation
{
  AL_POSTPROC_ROTATION_NONE = 0,
  AL_POSTPROC_R90,
  AL_POSTPROC_R180,
  AL_POSTPROC_R270,
}AL_EPostProcRotation;

typedef enum AL_EPostProcMirror
{
  AL_POSTPROC_MIRROR_NONE = 0,
  AL_POSTPROC_MIRROR_HORIZONTAL = 0x01,
  AL_POSTPROC_MIRROR_VERTICAL = 0x02,
  AL_POSTPROC_MIRROR_HV = 0x03,
}AL_EPostProcMirror;

typedef enum AL_EPostProcInput
{
  AL_POSTPROC_INPUT_TILE,
  AL_POSTPROC_INPUT_RASTER,
  AL_POSTPROC_INPUT_RASTER_YUY2,
}AL_EPostProcInput;

typedef enum AL_EPostProcAddrType
{
  AL_POSTPROC_ADDR_PHYSICAL,
  AL_POSTPROC_ADDR_VIRTUAL,
}AL_EPostProcAddrType;

typedef enum AL_EChromaPositionInLumaSamples
{
  AL_POSTPROC_CHROMA_POS_TOP_LEFT,
  AL_POSTPROC_CHROMA_POS_LEFT,
  AL_POSTPROC_CHROMA_POS_BOTTOM_LEFT,
}AL_EChromaPositionInLumaSamples;
