// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "ConvSrc.hpp" // TFrameInfo

extern "C"
{
#include "lib_common/BufferAPI.h"
}

void StorePictureInRaster(uint8_t const* pSrcY, uint8_t const* pSrcU, uint8_t const* pSrcV, int32_t iSrcPitchY, int32_t iSrcPitchU, int32_t iSrcPitchV, TFrameInfo const& tFrameInfo, AL_TBuffer* pDst);
void StorePictureInYUY2Raster(uint8_t const* pSrcY, uint8_t const* pSrcU, uint8_t const* pSrcV, AL_TDimension const& srcDim, AL_EChromaMode eCMode, uint8_t uBitDepth, uint8_t* pOut);
