// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/TileInputLoader.h"

#include "LoadLumaTile.h"
#include "lib_common/Utils.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Planes.h"
#include "lib_common/RoundUp.h"
}

static int GetTileSize(int iTileHeight)
{
  return iTileHeight == 8 ? 512 : 256;
}

void StorePictureInTiles(uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, TFrameInfo const& tFrameInfo, AL_TDimension tileSize, uint8_t* pDstY, uint8_t* pDstC1, uint8_t* pDstC2)
{
  auto pYTile = reinterpret_cast<uint16_t*>(pDstY);
  auto pC1Tile = reinterpret_cast<uint16_t*>(pDstC1);
  auto pC2Tile = reinterpret_cast<uint16_t*>(pDstC2);

  auto const iTileWidth = tileSize.iWidth;
  auto const iTileHeight = tileSize.iHeight;
  auto const bParity = (iTileHeight == 8) ? 0 : 1;

  for(int iY = 0; iY < AL_RoundUp(tFrameInfo.tDimension.iHeight, 8) / iTileHeight; iY++)
  {
    for(int iX = 0; iX < AL_RoundUp(tFrameInfo.tDimension.iWidth, 64) / iTileWidth; iX++)
    {
      // for picture widths not multiple of tile width
      uint8_t uTileSourceWidth = Clip3(tFrameInfo.tDimension.iWidth - (iX * iTileWidth), 0, iTileWidth);
      uint8_t uTileSourceHeight = Clip3(tFrameInfo.tDimension.iHeight - (iY * iTileHeight), 0, iTileHeight);

      auto LoadOnePlanarTile = [&tFrameInfo, iTileWidth, iTileHeight, uTileSourceWidth, uTileSourceHeight, iX, iY](uint8_t* pSrc, uint16_t* pTile)
                               {
                                 uint32_t const uOffset = (iY * iTileHeight) * tFrameInfo.tDimension.iWidth + (iX * iTileWidth);

                                 if(tFrameInfo.iBitDepth > 8)
                                 {
                                   auto pSrc16b = reinterpret_cast<uint16_t*>(pSrc) + uOffset;
                                   LoadLumaTile(pSrc16b, pTile, tFrameInfo.tDimension.iWidth, uTileSourceWidth, uTileSourceHeight, iTileWidth, iTileHeight);
                                 }
                                 else
                                 {
                                   auto pSrc8b = pSrc + uOffset;
                                   LoadLumaTile(pSrc8b, pTile, tFrameInfo.tDimension.iWidth, uTileSourceWidth, uTileSourceHeight, iTileWidth, iTileHeight);
                                 }
                               };

      LoadOnePlanarTile(pSrcY, pYTile);
      pYTile += GetTileSize(iTileHeight);

      if(tFrameInfo.eCMode == AL_CHROMA_4_4_4)
      {
        LoadOnePlanarTile(pSrcU, pC1Tile);
        pC1Tile += GetTileSize(iTileHeight);
        LoadOnePlanarTile(pSrcV, pC2Tile);
        pC2Tile += GetTileSize(iTileHeight);
      }
      else if((tFrameInfo.eCMode == AL_CHROMA_4_2_2) || ((tFrameInfo.eCMode == AL_CHROMA_4_2_0) && ((iY & 1) == bParity))) // one row out of two in 4:2:0
      {
        int iYC = tFrameInfo.eCMode == AL_CHROMA_4_2_2 ? iY : iY >> 1;
        uint32_t const uOffset = (iYC * iTileHeight) * AL_RoundUp(tFrameInfo.tDimension.iWidth, 2) / 2 + (iX * (iTileWidth / 2));

        auto uTileSourceHeight = iTileHeight;

        if(tFrameInfo.eCMode == AL_CHROMA_4_2_0 && !bParity)
          uTileSourceHeight = Clip3(AL_RoundUp(tFrameInfo.tDimension.iHeight, 2) / 2 - (iY / 2 * iTileHeight), 0, iTileHeight);

        if(tFrameInfo.iBitDepth > 8)
        {
          auto pSrc16bU = reinterpret_cast<uint16_t*>(pSrcU) + uOffset;
          auto pSrc16bV = reinterpret_cast<uint16_t*>(pSrcV) + uOffset;
          LoadChromaTile(pSrc16bU, pSrc16bV, pC1Tile, AL_RoundUp(tFrameInfo.tDimension.iWidth, 2) / 2, uTileSourceWidth / 2, iTileHeight, uTileSourceHeight, iTileWidth == 32);
        }
        else
        {
          auto pSrc8bU = pSrcU + uOffset;
          auto pSrc8bV = pSrcV + uOffset;
          LoadChromaTile(pSrc8bU, pSrc8bV, pC1Tile, AL_RoundUp(tFrameInfo.tDimension.iWidth, 2) / 2, uTileSourceWidth / 2, iTileHeight, uTileSourceHeight, iTileWidth == 32);
        }
        pC1Tile += GetTileSize(iTileHeight);
      }
    }
  }
}
