// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/PicFormat.h"
#include "lib_rtos/lib_rtos.h"

/****************************************************************************/
/* This function is intended to be used only for internal buffers, which cannot exist
  in packed format.
  Internal buffers configured with 420 & 422 chroma modes are stored in interleaved.
*/
AL_EPlaneMode GetInternalBufPlaneMode(AL_EChromaMode eChromaMode)
{
  return eChromaMode == AL_CHROMA_MONO ? AL_PLANE_MODE_MONOPLANE :
         (eChromaMode == AL_CHROMA_4_4_4 ? AL_PLANE_MODE_PLANAR : AL_PLANE_MODE_SEMIPLANAR);
}

/****************************************************************************/
AL_ESamplePackMode GetInternalBufSamplePackMode(AL_EFbStorageMode eFbStorageMode, uint8_t uBitDepth)
{
  (void)uBitDepth;

  if(IsTile(eFbStorageMode))
    return AL_SAMPLE_PACK_MODE_PACKED;

  return uBitDepth == 10 ? AL_SAMPLE_PACK_MODE_PACKED_XV : AL_SAMPLE_PACK_MODE_BYTE;
}

/*****************************************************************************/
bool IsTile(AL_EFbStorageMode eStorageMode)
{
  bool bIsTile = AL_FB_TILE_32x4 == eStorageMode || AL_FB_TILE_64x4 == eStorageMode;

  return bIsTile;
}

/*****************************************************************************/
bool Is10bPacked(AL_ESamplePackMode eSamplePackMode)
{
  return eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED ||
         eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED_XV;
}

/*****************************************************************************/
int32_t GetTileWidth(AL_EFbStorageMode eMode, uint8_t uBitDepth)
{
  (void)uBitDepth;

  if(eMode == AL_FB_TILE_32x4
     )
    return 32;

  if(eMode == AL_FB_TILE_64x4
     )
    return 64;

  return 0;
}

/*****************************************************************************/
int32_t GetTileHeight(AL_EFbStorageMode eMode)
{
  if(eMode == AL_FB_TILE_32x4 || eMode == AL_FB_TILE_64x4)
    return 4;

  return 0;
}

/*****************************************************************************/
int32_t GetTileSize(AL_EFbStorageMode eMode, uint8_t uBitDepth)
{

  return GetTileHeight(eMode) * GetTileWidth(eMode, uBitDepth) * uBitDepth / 8;
}

/*****************************************************************************/

bool IsRgbComponentOrder(AL_EComponentOrder eComponentOrder)
{
  return eComponentOrder >= AL_COMPONENT_ORDER_RGB && eComponentOrder <= AL_COMPONENT_ORDER_BGR;
}

/*****************************************************************************/
AL_TPicFormat GetDefaultPicFormat(void)
{
  AL_TPicFormat picFormat =
  {
    /* MAX_ENUM are filled in later */
    AL_CHROMA_MAX_ENUM,
    AL_ALPHA_MODE_DISABLED,
    0,
    AL_FB_RASTER,
    AL_PLANE_MODE_MAX_ENUM,
    AL_COMPONENT_ORDER_YUV,
    AL_SAMPLE_PACK_MODE_BYTE,
    false,
    false,
  };
  return picFormat;
}

char const* AL_ChromaModeToString(AL_EChromaMode eChromaMode)
{
  switch(eChromaMode)
  {
  case AL_CHROMA_MONO:
    return "Monochrome";
  case AL_CHROMA_4_2_0:
    return "420";
  case AL_CHROMA_4_2_2:
  case AL_CHROMA_4_2_2_HORIZONTAL:
    return "422";
  case AL_CHROMA_4_4_4:
    return "444";
  case AL_CHROMA_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown chroma mode";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_AlphaModeToString(AL_EAlphaMode eAlphaMode)
{
  switch(eAlphaMode)
  {
  case AL_ALPHA_MODE_DISABLED:
    return "No alpha";
  case AL_ALPHA_MODE_BEFORE:
    return "Alpha before";
  case AL_ALPHA_MODE_AFTER:
    return "Alpha before";
  case AL_ALPHA_MODE_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown alpha mode";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_FbStorageModeToString(AL_EFbStorageMode eStorageMode)
{
  switch(eStorageMode)
  {
  case AL_FB_RASTER:
    return "Raster";
  case AL_FB_TILE_32x4:
    return "Tile 32x4";
  case AL_FB_TILE_64x4:
    return "Tile 64x4";
  case AL_FB_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown storage mode";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_PlaneModeToString(AL_EPlaneMode ePlaneMode)
{
  switch(ePlaneMode)
  {
  case AL_PLANE_MODE_PLANAR:
    return "Planar";
  case AL_PLANE_MODE_SEMIPLANAR:
    return "Semiplanar";
  case AL_PLANE_MODE_INTERLEAVED:
    return "Interleaved";
  case AL_PLANE_MODE_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown plane mode";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_ComponentOrderToString(AL_EComponentOrder eComponentOrder)
{
  switch(eComponentOrder)
  {
  case AL_COMPONENT_ORDER_YUV:
    return "YUV";
  case AL_COMPONENT_ORDER_YVU:
    return "YVU";
  case AL_COMPONENT_ORDER_UYV:
    return "UYV";
  case AL_COMPONENT_ORDER_UVY:
    return "UVY";
  case AL_COMPONENT_ORDER_VYU:
    return "VYU";
  case AL_COMPONENT_ORDER_VUY:
    return "VUY";
  case AL_COMPONENT_ORDER_YUYV:
    return "YUYV";
  case AL_COMPONENT_ORDER_UYVY:
    return "UYVY";
  case AL_COMPONENT_ORDER_RGB:
    return "RGB";
  case AL_COMPONENT_ORDER_RBG:
    return "RBG";
  case AL_COMPONENT_ORDER_GRB:
    return "GRB";
  case AL_COMPONENT_ORDER_GBR:
    return "GBR";
  case AL_COMPONENT_ORDER_BRG:
    return "BRG";
  case AL_COMPONENT_ORDER_BGR:
    return "BGR";
  case AL_COMPONENT_ORDER_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown component order";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_SamplePackModeToString(AL_ESamplePackMode eSamplePackMode)
{
  switch(eSamplePackMode)
  {
  case AL_SAMPLE_PACK_MODE_BYTE:
    return "Byte";
  case AL_SAMPLE_PACK_MODE_PACKED:
    return "Packed";
  case AL_SAMPLE_PACK_MODE_PACKED_XV:
    return "Packed XV";
  case AL_SAMPLE_PACK_MODE_MAX_ENUM:
  default:
  {
    char const* sUnknownAlphaMode = "Unknown sample pack mode";
    Rtos_AssertWithMessage(false, sUnknownAlphaMode, __FILE__, __LINE__);
    return sUnknownAlphaMode;
  }
  }
}

char const* AL_CompressedToString(bool bIsCompressed)
{
  return bIsCompressed ? "Compressed" : "Not compressed";
}

char const* AL_MsbToString(bool bIsMSB)
{
  return bIsMSB ? "MSB format" : "LSB format";
}
