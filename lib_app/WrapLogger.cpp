// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/WrapLogger.hpp"

Logger::Logger(const std::string& outputFile, AL_ITimer* timer) :
  outputFile{outputFile}
{
  if(outputFile.empty())
    maxCount = 0;

  samples.resize(maxCount);
  events.samples = samples.data();
  events.max = maxCount;
  events.count = 0;

  logger = AL_DefaultLogger_Init(AL_GetDefaultAllocator(), timer, &events);
}

Logger::~Logger()
{
  AL_ILogger_Deinit(logger);

  if(outputFile.empty())
    return;

  std::ofstream tracer(outputFile);

  for(int32_t i = 0; i < events.count; i++)
    tracer << std::string(samples[i].label) << " " << samples[i].timestamp << std::endl;
}

AL_ILogger* Logger::GetLogger()
{
  return logger;
}
