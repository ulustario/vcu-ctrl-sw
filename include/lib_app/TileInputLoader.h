// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "ConvSrc.h"

void StorePictureInTiles(uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, TFrameInfo const& TFrameInfo, AL_TDimension tileSize, uint8_t* pDstY, uint8_t* pDstC1, uint8_t* pDstC2);
