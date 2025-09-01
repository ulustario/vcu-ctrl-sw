// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
extern "C"
{
#include "lib_log/LoggerDefault.h"
#include "lib_log/TimerInterface.h"
}
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

struct Logger
{
  Logger(std::string const& outputFile, AL_ITimer* timer);
  ~Logger(void);

  AL_ILogger* GetLogger(void);

private:
  int32_t maxCount = 32000;
  std::vector<AL_TDefaultLoggerSample> samples;
  AL_TDefaultLoggerEvents events;
  std::string outputFile;
  AL_ILogger* logger;
};
