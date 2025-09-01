// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common_dec/StreamSettings.h"
#include <stdbool.h>

#define AL_REF_MNGR_CONCEAL_BUF 1
#define AL_REF_MNGR_REC_BUF 1

/******************************************************************************/
int32_t AL_AVC_GetMaxDpbBuffers(AL_TStreamSettings const* pCurrentStreamSettings, int32_t iSPSMaxRefFrames);

/******************************************************************************/
int32_t AVC_GetMinOutputBuffersNeeded(int32_t iDpbMaxBuf, int32_t iStack, bool bDecodeIntraOnly);

/*****************************************************************************/
int32_t AL_HEVC_GetMaxDpbBuffers(AL_TStreamSettings const* pCurrentStreamSettings);
