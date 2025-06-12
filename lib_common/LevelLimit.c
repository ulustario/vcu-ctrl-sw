// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "LevelLimit.h"

/*************************************************************************/
uint8_t AL_GetRequiredLevel(uint32_t uVal, const AL_TLevelLimit* pLevelLimits, int32_t iNbLimits)
{
  for(int32_t i = 0; i < iNbLimits; i++)
  {
    if(uVal <= pLevelLimits[i].uLimit)
      return pLevelLimits[i].uLevel;
  }

  return 255u;
}
