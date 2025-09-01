// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <map>
#include <stdexcept>

extern "C"
{
#include "lib_common/PicFormat.h"
#include "lib_common_postproc/PostProcTypes.h"
#include "lib_common/FourCC.h"
#include "lib_common/AllocatorTracker.h"
}

static const std::map<std::string, AL_EPostProcRotation> HandledRotation =
{
  { "0", AL_POSTPROC_ROTATION_NONE },
  { "90", AL_POSTPROC_R90 },
  { "180", AL_POSTPROC_R180 },
  { "270", AL_POSTPROC_R270 },
};

static const std::map<std::string, AL_EPostProcMirror> HandledMirror =
{
  { "NONE", AL_POSTPROC_MIRROR_NONE },
  { "H", AL_POSTPROC_MIRROR_HORIZONTAL },
  { "V", AL_POSTPROC_MIRROR_VERTICAL },
  { "HV", AL_POSTPROC_MIRROR_HV },
  { "VH", AL_POSTPROC_MIRROR_HV },
};

static const std::map<std::string, AL_EChromaMode> HandledChromaMode =
{
  { "400", AL_CHROMA_4_0_0 },
  { "420", AL_CHROMA_4_2_0 },
  { "422", AL_CHROMA_4_2_2 },
  { "444", AL_CHROMA_4_4_4 },
};

static const std::map<std::string, AL_EFbStorageMode> HandledStorageModes =
{
  { "raster", AL_FB_RASTER },
  { "tile32x4", AL_FB_TILE_32x4 },
  { "tile64x4", AL_FB_TILE_64x4 },
};

/******************************************************************************/

template<typename T>
std::string GetHandledValuesList(const std::map<std::string, T> HandledValues)
{
  std::string sHandledValuesList;

  for(const auto& bdMode : HandledValues)
    sHandledValuesList += std::string("'") + bdMode.first + std::string("' ");

  return sHandledValuesList;
}

template<typename T>
T ParseHandledValue(std::string s, const std::map<std::string, T> HandledValues, T defaultValue)
{
  if(s.empty())
  {
    return defaultValue;
  }

  auto chosenValue = HandledValues.find(s);

  if(chosenValue != HandledValues.end())
    return chosenValue->second;

  throw std::runtime_error(std::string("Wrong value. Allowed values are: ") + GetHandledValuesList(HandledValues));
}

AL_EFbStorageMode ParseFrameBufferFormat(const std::string& sBufFormat, bool& bBufComp);
std::string GetFrameBufferFormatOptDesc(bool bSecondOutput = false);
AL_TPosition ParsePosition(std::string s, int32_t iMultiple);
AL_TDimension ParseDimension(std::string s, int32_t multiple);
