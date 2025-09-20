// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <string>
#include <memory>

extern "C"
{
#include "lib_log/LoggerInterface.h"
#include "lib_log/TimerInterface.h"
}

struct Logger
{
  Logger(std::string outputFile, AL_ITimer* timer);
  ~Logger(void);

  AL_ILogger* GetLogger(void);

private:
  struct WrapLogger;
  std::unique_ptr<WrapLogger> const pWrapLogger;
};
