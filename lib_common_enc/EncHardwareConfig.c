// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncHardwareConfig.h"

AL_EChromaMode AL_HWConfig_Enc_GetSupportedChromaMode(AL_EProfile eProfile)
{
  (void)eProfile;
  return AL_CHROMA_4_2_2;
}

int32_t AL_HWConfig_Enc_GetSupportedBitDepth(AL_EProfile eProfile)
{
  (void)eProfile;
  return 10;
}

int32_t AL_HWConfig_Enc_GetSupportedL2PBitDepth(void)
{
  return 10;
}

