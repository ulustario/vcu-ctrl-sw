// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/Profiles.h"

typedef struct
{
  int32_t minWidth;
  int32_t maxWidth;
  int32_t lcuSize;
  int32_t resources;
  int32_t cycles32x32[4];
  bool enableMultiCore;
}AL_CoreConstraint;

void AL_CoreConstraint_Init(AL_CoreConstraint* constraint, int32_t coreFrequency, int32_t margin, uint32_t const* hardwareCyclesCounts, int32_t minWidth, int32_t maxWidth, int32_t lcuSize);
int32_t AL_CoreConstraint_GetExpectedNumberOfCores(AL_CoreConstraint const* constraint, int32_t width, int32_t height, int32_t chromaModeIdc, int32_t frameRate, int32_t clockRatio);
int32_t AL_CoreConstraint_GetMinCoresCount(AL_CoreConstraint const* constraint, int32_t width);

AL_64U AL_GetResources(int32_t width, int32_t height, int32_t frameRate, int32_t clockRatio, int32_t cycles32x32);

/* Doesn't support NUMCORE_AUTO, only works on actual number of cores. */
typedef struct
{
  int32_t requiredWidthInCtbPerCore;
  int32_t actualWidthInCtbPerCore; /* calculated without taking the alignment of the cores in consideration */
}AL_NumCoreDiagnostic;

bool AL_Constraint_NumTileIsSane(AL_ECodec codec, int32_t width, int32_t numTile, int32_t log2MaxCuSize, AL_NumCoreDiagnostic* diagnostic);
