// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/RasterInputLoader.hpp"
#include <cstring>
#include <cassert>
#include "lib_app/PixMapBufPool.hpp"

namespace
{
void CopyPlanarComponent(uint8_t const* pSrc, uint8_t* pDst, int32_t iSrcPitch, int32_t iDstPitch, int32_t iHeight, int32_t iLineDataSize)
{
  for(auto iY = 0; iY < iHeight; ++iY, pDst += iDstPitch, pSrc += iSrcPitch)
    memcpy(pDst, pSrc, iLineDataSize);
}

template<typename T>
void InterlacedChroma(T const* pU, T const* pV, int32_t iSrcPitch, AL_TDimension const& tDimension, T* pOutC, int32_t iPitch)
{
  for(auto iY = 0; iY < tDimension.iHeight; ++iY)
  {
    for(auto iX = 0; iX < tDimension.iWidth; ++iX, ++pU, ++pV, pOutC += 2)
    {
      pOutC[0] = pU[0];
      pOutC[1] = pV[0];
    }

    pOutC += (iPitch / sizeof(T)) - tDimension.iWidth * 2;
    pU += (iSrcPitch / sizeof(T)) - tDimension.iWidth;
    pV += (iSrcPitch / sizeof(T)) - tDimension.iWidth;
  }
}
}

void StorePictureInRaster(uint8_t const* pSrcY, uint8_t const* pSrcU, uint8_t const* pSrcV, int32_t iSrcPitchY, int32_t iSrcPitchU, int32_t iSrcPitchV, TFrameInfo const& tFrameInfo, AL_TBuffer* pDst)
{
  auto const pixelSize = tFrameInfo.iBitDepth > 8 ? sizeof(uint16_t) : sizeof(uint8_t);
  auto const size = tFrameInfo.tDimension.iWidth * pixelSize;

  uint8_t* pY = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  uint8_t* pC1 = nullptr;
  uint8_t* pC2 = nullptr;

  AL_EPlaneMode eOutputPlaneMode = AL_GetPlaneMode(AL_PixMapBuffer_GetFourCC(pDst));

  if(AL_PLANE_MODE_SEMIPLANAR == eOutputPlaneMode)
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  }
  else
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_U);
    pC2 = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_V);
  }
  int32_t iPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);
  CopyPlanarComponent(pSrcY, pY, iSrcPitchY, iPitch, tFrameInfo.tDimension.iHeight, size);

  if(tFrameInfo.eCMode == AL_CHROMA_MONO)
    return;

  if(tFrameInfo.eCMode == AL_CHROMA_4_4_4)
  {
    iPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_U);
    CopyPlanarComponent(pSrcU, pC1, iSrcPitchU, iPitch, tFrameInfo.tDimension.iHeight, size);
    iPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_V);
    CopyPlanarComponent(pSrcV, pC2, iSrcPitchV, iPitch, tFrameInfo.tDimension.iHeight, size);
    return;
  }

  AL_TDimension Cdim = { 0, 0 };
  switch(tFrameInfo.eCMode)
  {
  case AL_CHROMA_4_2_2:
    Cdim = { (tFrameInfo.tDimension.iWidth + 1) / 2, tFrameInfo.tDimension.iHeight };
    break;
  case AL_CHROMA_4_2_0:
    Cdim = { (tFrameInfo.tDimension.iWidth + 1) / 2, (tFrameInfo.tDimension.iHeight + 1) / 2 };
    break;
  default:
    assert(0);
  }

  iPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  if(tFrameInfo.iBitDepth > 8)
  {
    auto const pInU = reinterpret_cast<uint16_t const*>(pSrcU);
    auto const pInV = reinterpret_cast<uint16_t const*>(pSrcV);
    auto pOutC = reinterpret_cast<uint16_t*>(pC1);
    InterlacedChroma(pInU, pInV, iSrcPitchU, Cdim, pOutC, iPitch);
  }
  else
    InterlacedChroma(pSrcU, pSrcV, iSrcPitchV, Cdim, pC1, iPitch);
}

namespace
{
template<typename T>
void I422_To_YUY2_Line(T const* pSrcY, T const* pSrcU, T const* pSrcV, T* pOut, int32_t iWidth)
{
  for(auto iX = 0; iX < iWidth; iX += 2, pSrcY += 2, ++pSrcU, ++pSrcV, pOut += 4)
  {
    pOut[0] = pSrcY[0];
    pOut[1] = pSrcU[0];
    pOut[2] = pSrcY[1];
    pOut[3] = pSrcV[0];
  }
}

template<typename T>
void Y400_To_YUY2(T const* pSrcY, AL_TDimension const& tDimension, T* pOut)
{
  for(auto iY = 0; iY < tDimension.iHeight; ++iY)
    for(auto iX = 0; iX < tDimension.iWidth; ++iX, ++pSrcY, pOut += 2)
    {
      pOut[0] = pSrcY[0];
      pOut[1] = 0x80; // Upscale to 422 with default value for U & V
    }
}

template<typename T>
void Y420_To_YUY2(T const* pSrcY, T const* pSrcU, T const* pSrcV, AL_TDimension const& tDimension, T* pOut)
{
  for(auto iY = 0; iY < tDimension.iHeight; iY += 2)
  {
    I422_To_YUY2_Line(pSrcY, pSrcU, pSrcV, pOut, tDimension.iWidth);
    pSrcY += tDimension.iWidth;
    pOut += 2 * tDimension.iWidth;
    // Upscale chroma to 422 by using same U & V lines twice
    I422_To_YUY2_Line(pSrcY, pSrcU, pSrcV, pOut, tDimension.iWidth);
    pSrcY += tDimension.iWidth;
    pSrcU += tDimension.iWidth / 2;
    pSrcV += tDimension.iWidth / 2;
    pOut += 2 * tDimension.iWidth;
  }
}

template<typename T>
void Y422_To_YUY2(T const* pSrcY, T const* pSrcU, T const* pSrcV, AL_TDimension const& tDimension, T* pOut)
{
  for(auto iY = 0; iY < tDimension.iHeight; ++iY)
  {
    I422_To_YUY2_Line(pSrcY, pSrcU, pSrcV, pOut, tDimension.iWidth);
    pSrcY += tDimension.iWidth;
    pSrcU += tDimension.iWidth / 2;
    pSrcV += tDimension.iWidth / 2;
    pOut += 2 * tDimension.iWidth;
  }
}

template<typename T>
void ToYUY2Raster(T const* pSrcY, T const* pSrcU, T const* pSrcV, AL_TDimension const& tDimension, AL_EChromaMode eCMode, T* pOut)
{
  switch(eCMode)
  {
  case AL_CHROMA_4_2_2:
    Y422_To_YUY2(pSrcY, pSrcU, pSrcV, tDimension, pOut);
    break;
  case AL_CHROMA_4_2_0:
    Y420_To_YUY2(pSrcY, pSrcU, pSrcV, tDimension, pOut);
    break;
  case AL_CHROMA_4_0_0:
    Y400_To_YUY2(pSrcY, tDimension, pOut);
    break;
  default:
    assert(false);
  }
}
}

void StorePictureInYUY2Raster(uint8_t const* pSrcY, uint8_t const* pSrcU, uint8_t const* pSrcV, AL_TDimension const& tDimension, AL_EChromaMode eCMode, uint8_t iBitDepth, uint8_t* pOut)
{
  if(iBitDepth > 8)
  {
    auto const pInY = reinterpret_cast<uint16_t const*>(pSrcY);
    auto const pInU = reinterpret_cast<uint16_t const*>(pSrcU);
    auto const pInV = reinterpret_cast<uint16_t const*>(pSrcV);
    auto pBufOut = reinterpret_cast<uint16_t*>(pOut);
    ToYUY2Raster(pInY, pInU, pInV, tDimension, eCMode, pBufOut);
  }
  else
    ToYUY2Raster(pSrcY, pSrcU, pSrcV, tDimension, eCMode, pOut);
}
