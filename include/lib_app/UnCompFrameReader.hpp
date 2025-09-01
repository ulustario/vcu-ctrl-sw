// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_app/FrameReader.hpp"
#include "lib_app/YuvIO.hpp"

class UnCompFrameReader : public FrameReader
{
public:
  UnCompFrameReader(std::ifstream& File, AL_TYUVFileInfo& tFileInfo, bool bLoopFrames);
  virtual bool ReadFrame(AL_TBuffer* pFrameBuffer);

  void SeekAbsolute(uint32_t uFrameIdx);
  void SeekRelative(int32_t iFrameIdxDelta);

  void SetRndDim(uint32_t uRndDim) { m_uRndDim = uRndDim; };

private:
  AL_TYUVFileInfo& m_tFileInfo;
  uint32_t m_uRndDim;
};
