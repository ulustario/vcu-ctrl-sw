// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "LevelLimit.h"

bool AL_HEVC_CheckLevel(int32_t level);
uint32_t AL_HEVC_GetMaxNumberOfSlices(int32_t level);
uint32_t AL_HEVC_GetMaxTileColumns(int32_t level);
uint32_t AL_HEVC_GetMaxTileRows(int32_t level);
uint32_t AL_HEVC_GetMaxCPBSize(int32_t level, int32_t tier);
uint32_t AL_HEVC_GetMaxDPBSize(int32_t iLevel, int32_t iWidth, int32_t iHeight, bool bIsIntraProfile, bool bIsStillProfile, bool bDecodeIntraOnly);

uint8_t AL_HEVC_GetLevelFromFrameSize(int32_t numPixPerFrame);
uint8_t AL_HEVC_GetLevelFromPixRate(int32_t pixRate);
uint8_t AL_HEVC_GetLevelFromBitrate(int32_t bitrate, int32_t tier);
uint8_t AL_HEVC_GetLevelFromTileCols(int32_t tileCols);
uint8_t AL_HEVC_GetLevelFromDPBSize(int32_t dpbSize, int32_t pixRate);
