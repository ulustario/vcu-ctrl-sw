// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/PicFormat.h"

/****************************************************************************/
AL_EChromaOrder GetChromaOrder(AL_EChromaMode eChromaMode)
{
  return eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA :
         (eChromaMode == AL_CHROMA_4_4_4 ? AL_C_ORDER_U_V : AL_C_ORDER_SEMIPLANAR);
}

/****************************************************************************/
bool IsRaster(AL_EFbStorageMode eFBStorageMode)
{
  return eFBStorageMode == AL_FB_RASTER
  ;
}

/*****************************************************************************/
int GetTileWidth(AL_EFbStorageMode eMode)
{
  if(eMode == AL_FB_TILE_32x4
     )
    return 32;

  if(eMode == AL_FB_TILE_64x4
     )
    return 64;

  return 0;
}

/*****************************************************************************/
int GetTileHeight(AL_EFbStorageMode eMode)
{
  if(eMode == AL_FB_TILE_32x4 || eMode == AL_FB_TILE_64x4)
    return 4;

  return 0;
}

/*****************************************************************************/
int GetTileSize(AL_EFbStorageMode eMode, uint8_t uBitDepth)
{
  return GetTileHeight(eMode) * GetTileWidth(eMode) * uBitDepth / 8;
}
