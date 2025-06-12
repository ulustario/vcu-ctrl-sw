// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <iostream>
#include <stdexcept>
#include <fstream>

extern "C"
{
#include "lib_common/PicFormat.h"
#include "lib_common/BufferAPI.h"
#include "lib_rtos/types.h"
}

class FrameReader
{
protected:
  std::ifstream& m_recFile;
  bool m_bLoopFile;
  int32_t m_uTotalFrameCount;

  FrameReader(std::ifstream& iRecFile, bool bLoopFrames) :
    m_recFile(iRecFile),
    m_bLoopFile(bLoopFrames),
    m_uTotalFrameCount(0) {};

public:
  inline int32_t GetTotalFrameCnt() const { return m_uTotalFrameCount; }

  virtual bool ReadFrame(AL_TBuffer* pFrameBuffer) = 0;

  virtual void SeekAbsolute(uint32_t uFrameIdx) = 0;
  virtual void SeekRelative(int32_t iFrameIdxDelta) = 0;

  int32_t GotoNextPicture(int32_t iFileFrameRate, int32_t iEncFrameRate, int32_t iFilePictCount, int32_t iEncPictCount);

  size_t GetFileSize();

  virtual ~FrameReader() = default;
};
