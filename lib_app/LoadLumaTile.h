// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <algorithm>

template<typename T>
void LoadLumaTile(T pInY, uint16_t* Tile, int32_t iPitch, int32_t iWidth, int32_t iHeight, int32_t iTileWidth, int32_t iTileHeight)
{
  auto const S1 = iTileWidth == 32 ? 7 : 8;

  for(int32_t iY = 0; iY < iTileHeight; ++iY)
  {
    for(int32_t iX = 0; iX < iTileWidth; ++iX)
    {
      bool bPadding = iX >= iWidth;
      Tile[((iY >> 2) << S1) + ((iX >> 2) << 4) + ((iY & 0x3) << 2) + (iX & 3)] = (bPadding ? pInY[-1] : *pInY);

      if(!bPadding)
        pInY++;
    }

    bool bPadding = iY >= (iHeight - 1);

    if(!bPadding)
      pInY += iPitch - iWidth;
    else
      pInY -= iWidth;
  }
}

template<typename T>
void LoadChromaTile(T pInU, T pInV, uint16_t* Tile, int32_t iPitch, int32_t iWidth, int32_t iTileHeight, int32_t iHeight, bool bTileWidth32)
{
  auto const iTileSize = bTileWidth32 ? 16 : 32;
  auto const S1 = bTileWidth32 ? 7 : 8;

  for(int32_t iY = 0; iY < iTileHeight; ++iY)
  {
    bool bVertPadding = iY >= iHeight;

    for(int32_t iX = 0; iX < iTileSize; ++iX)
    {
      bool bHorzPadding = iX >= iWidth;
      auto const iIdx = ((iY >> 2) << S1) + ((iX / 4) * 32) + ((iY & 0x3) * 4) + (iX & 3);

      if(bVertPadding)
      {
        if(bHorzPadding)
        {
          Tile[iIdx] = pInU[-iPitch - 1];
          Tile[iIdx + 16] = pInV[-iPitch - 1];
        }
        else
        {
          Tile[iIdx] = pInU[-iPitch];
          Tile[iIdx + 16] = pInV[-iPitch];
          ++pInU;
          ++pInV;
        }
      }
      else if(bHorzPadding)
      {
        Tile[iIdx] = pInU[-1];
        Tile[iIdx + 16] = pInV[-1];
      }
      else
      {
        Tile[iIdx] = *pInU;
        Tile[iIdx + 16] = *pInV;
        ++pInU;
        ++pInV;
      }
    }

    if(!bVertPadding)
    {
      pInU += iPitch - iWidth;
      pInV += iPitch - iWidth;
    }
    else
    {
      pInU -= iWidth;
      pInV -= iWidth;
    }
  }
}
