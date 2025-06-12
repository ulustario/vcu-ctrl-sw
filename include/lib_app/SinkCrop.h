// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <lib_app/Sink.h>

extern "C"
{
#include "lib_common/PicFormat.h"
}

struct SinkCrop final : IFrameSink
{
  SinkCrop(std::unique_ptr<IFrameSink>& pSink, AL_TCropInfo* pCropInfo = nullptr);

  void ProcessFrame(AL_TBuffer* pBuf) override;

private:
  void ApplyCrop(AL_TBuffer* pYUV, int32_t iSizePix, int32_t iLeft, int32_t iRight, int32_t iTop, int32_t iBottom);

  std::unique_ptr<IFrameSink> m_pSink;
  bool m_bFixedCrop = false;
  AL_TCropInfo m_tCrop = { false, 0, 0, 0, 0 };
};
