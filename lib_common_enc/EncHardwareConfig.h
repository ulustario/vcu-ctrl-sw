// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Profiles.h"

AL_EChromaMode AL_HWConfig_Enc_GetSupportedChromaMode(AL_EProfile eProfile);
int32_t AL_HWConfig_Enc_GetSupportedBitDepth(AL_EProfile eProfile);
int32_t AL_HWConfig_Enc_GetSupportedL2PBitDepth(void);
