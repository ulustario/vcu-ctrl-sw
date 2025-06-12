// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#define AL_MAX_SLICE_HEADER_SIZE 512

/****************************************************************************/
int32_t GetPcmVclNalSize(AL_TDimension tDim, AL_EChromaMode eMode, int32_t iBitDepth);

/****************************************************************************/
int32_t GetPCMSize(int32_t uNumLCU, uint8_t uLog2MaxCuSize, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIntermediateBuffer);
