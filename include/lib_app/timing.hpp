// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>
#include <thread>

extern "C"
{
#include "lib_rtos/types.h"
}

static inline AL_64U GetPerfTime(void)
{
  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
}

static inline void Sleep(int32_t ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
