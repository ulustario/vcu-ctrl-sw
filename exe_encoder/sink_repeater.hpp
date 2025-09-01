// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "IEncoderSink.hpp"
#include <vector>
#include <stdexcept>

struct RepeaterSink : IEncoderSink
{
  RepeaterSink(IEncoderSink* pNext, int32_t bufferingCount, int32_t maxPicCount) :
    m_pNext(pNext), m_bufferingCount{bufferingCount}, m_picCount{maxPicCount}
  {
  }

  ~RepeaterSink(void)
  {
    for(auto frame : m_frames)
      AL_Buffer_Unref(frame);
  }

  void SetChangeSourceCallback(ChangeSourceCallback changeSourceCB) override
  {
    (void)changeSourceCB;
  }

  void PreprocessFrame() override {};

  void startProcessForReal(void)
  {
    auto frame = m_frames.begin();

    while(m_picCount > 0)
    {
      m_pNext->PreprocessFrame();
      m_pNext->ProcessFrame(*frame);
      --m_picCount;
      ++frame;

      if(frame == m_frames.end())
        frame = m_frames.begin();
    }

    m_pNext->PreprocessFrame();
    m_pNext->ProcessFrame(nullptr);
  }

  void ProcessFrame(AL_TBuffer* frame) override
  {
    if(frame)
    {
      AL_Buffer_Ref(frame);
      m_frames.push_back(frame);
      --m_bufferingCount;
    }
    else
    {
      m_bufferingCount = 0;
    }

    if(m_bufferingCount > 0 || m_hasAlreadyStarted)
      return;

    if(!m_hasAlreadyStarted)
    {
      m_hasAlreadyStarted = true;
      startProcessForReal();
    }
  }

  AL_ERR GetLastError(void) override
  {
    return m_pNext->GetLastError();
  }

private:
  IEncoderSink* m_pNext;
  int32_t m_bufferingCount;
  int32_t m_picCount;
  bool m_hasAlreadyStarted = false;

  std::vector<AL_TBuffer*> m_frames;

};
