// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "ConvSrc.h" // TFrameInfo

extern "C"
{
#include "lib_common/BufferAPI.h"
}

void StorePictureInRaster(uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, int iSrcPitchY, int iSrcPitchU, int iSrcPitchV, TFrameInfo const& tFrameInfo, AL_TBuffer* pDst);
void StorePictureInYUY2Raster(uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, AL_TDimension const& srcDim, AL_EChromaMode eCMode, uint8_t uBitDepth, uint8_t* pOut);
