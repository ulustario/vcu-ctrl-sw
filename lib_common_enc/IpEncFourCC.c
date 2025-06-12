// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_enc/IpEncFourCC.h"

#include "lib_common/FourCC.h"
#include "lib_common_enc/EncBuffers.h"

/****************************************************************************/
TFourCC AL_EncGetSrcFourCC(AL_TPicFormat const picFmt)
{
  if(AL_FB_RASTER == picFmt.eStorageMode)
  {
    Rtos_Assert(picFmt.eChromaMode == AL_CHROMA_MONO
                || picFmt.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR
                || (picFmt.eChromaMode == AL_CHROMA_4_4_4 && picFmt.ePlaneMode == AL_PLANE_MODE_PLANAR));
    Rtos_Assert(picFmt.uBitDepth == 8 || (AL_SAMPLE_PACK_MODE_PACKED_XV == picFmt.eSamplePackMode));
  }

  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetSrcPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, AL_ESrcMode eSrcMode)
{
  AL_ESamplePackMode eSamplePackMode = AL_SAMPLE_PACK_MODE_BYTE;
  bool bIsSourceYUV = true;
  AL_EAlphaMode eAlphaMode = AL_ALPHA_MODE_DISABLED;
  AL_EChromaMode eRealChromaMode = eChromaMode;
  AL_EComponentOrder eComponentOrder = bIsSourceYUV ? AL_COMPONENT_ORDER_YUV : AL_COMPONENT_ORDER_BGR;
  AL_EFbStorageMode eStorageMode = AL_GetSrcStorageMode(eSrcMode);

  bool bCompressed = AL_IsSrcCompressed(eSrcMode);
  bool bMSB = AL_IsSrcMSB(eSrcMode);
  bool bInterleaved = AL_IsSrcInterleaved(eSrcMode);

  if(eStorageMode == AL_FB_TILE_32x4 || eStorageMode == AL_FB_TILE_64x4)
    eSamplePackMode = AL_SAMPLE_PACK_MODE_PACKED;

  if(AL_FB_RASTER == eStorageMode && 10 == uBitDepth)
  {
    eSamplePackMode = AL_SAMPLE_PACK_MODE_PACKED_XV;
    bMSB = false;
  }

  AL_TPicFormat picFormat =
  {
    eRealChromaMode,
    eAlphaMode,
    uBitDepth,
    eStorageMode,
    bInterleaved ? AL_PLANE_MODE_INTERLEAVED : GetInternalBufPlaneMode(eChromaMode),
    eComponentOrder,
    eSamplePackMode,
    bCompressed,
    bMSB
  };
  return picFormat;
}

/****************************************************************************/
TFourCC AL_GetRecFourCC(AL_TPicFormat const picFmt)
{
  Rtos_Assert(picFmt.eStorageMode == AL_FB_TILE_64x4);
  return AL_GetFourCC(picFmt);
}

/****************************************************************************/
AL_TPicFormat AL_EncGetRecPicFormat(AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsCompressed, AL_EFbStorageMode eStorageMode)
{
  Rtos_Assert(eStorageMode & FB_TILE_64x4);

  AL_TPicFormat picFormat =
  {
    eChromaMode,
    AL_ALPHA_MODE_DISABLED,
    uBitDepth,
    eStorageMode,
    GetInternalBufPlaneMode(eChromaMode),
    AL_COMPONENT_ORDER_YUV,
    AL_SAMPLE_PACK_MODE_PACKED,
    bIsCompressed,
    false
  };
  return picFormat;
}
