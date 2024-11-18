// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

extern "C"
{
#include "lib_common/PicFormat.h"
}
#include <fstream>
#include <stdexcept>
#include <cassert>

static constexpr uint8_t CurrentCompFileVersion = 3;

enum ETileMode : uint8_t
{
  TILE_64x4_v0 = 0,
  TILE_64x4_v1 = 1,
  TILE_32x4_v1 = 2,
  RASTER = 5,
  TILE_MAX_ENUM,
};

ETileMode EFbStorageModeToETileMode(AL_EFbStorageMode eFbStorageMode, uint8_t uBitDepth);
AL_EFbStorageMode ETileModeToEFbStorageMode(ETileMode eTileMode);

static inline int GetTileSize(int iTileHeight)
{
  return iTileHeight == 8 ? 512 : 256;
}

static inline size_t GetOneChromaSize(size_t iLumaSize, AL_EChromaMode eCMode)
{
  switch(eCMode)
  {
  case AL_CHROMA_4_2_2:
    return iLumaSize / 2;
  case AL_CHROMA_4_2_0:
    return iLumaSize / 4;
  case AL_CHROMA_4_0_0:
    return 0;
  case AL_CHROMA_4_4_4:
    return iLumaSize;
  default:
    assert(false);
    return 0;
  }
}
