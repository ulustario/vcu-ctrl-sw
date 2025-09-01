// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/FrameReader.hpp"
#include "lib_app/FileUtils.hpp"

extern "C"
{
#include "lib_rtos/types.h"
}

size_t FrameReader::GetFileSize(void)
{
  size_t zSize;

  if(!::GetFileSize(m_recFile, zSize))
    throw std::runtime_error("Invalid YUV file");
  return zSize;
}

int32_t FrameReader::GotoNextPicture(int32_t iFileFrameRate, int32_t iEncFrameRate, int32_t iEncPictCount, int32_t iFilePictCount)
{
  const int32_t iMove = ((iEncPictCount * iFileFrameRate) / iEncFrameRate) - iFilePictCount;

  if(iMove)
    this->SeekRelative(iMove);

  return iMove;
}
