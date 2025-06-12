// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

typedef struct AL_TConceal
{
  bool bHasPPS;
  bool bValidFrame;
  int32_t iLastPPSId;
  int32_t iActivePPS;
  int32_t iFirstLCU;
  bool bSkipRemainingNals;
}AL_TConceal;

void AL_Conceal_Init(AL_TConceal* pConceal);
