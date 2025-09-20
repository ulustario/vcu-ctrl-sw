// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/WrapLogger.hpp"
#include "lib_app/utils.hpp"

#include <fstream>

using namespace std;

extern "C"
{
#include "lib_log/LoggerDefault.h"
}

struct Logger::WrapLogger
{
  int32_t maxCount = 32000;
  vector<AL_TDefaultLoggerSample> samples;
  AL_TDefaultLoggerEvents events;
  string outputFile;
  AL_ILogger* logger;
};

Logger::Logger(string outputFile, AL_ITimer* timer) :
  pWrapLogger{make_unique<Logger::WrapLogger>()}
{
  pWrapLogger->outputFile = outputFile;

  if(pWrapLogger->outputFile.empty())
    pWrapLogger->maxCount = 0;

  pWrapLogger->samples.resize(pWrapLogger->maxCount);
  pWrapLogger->events.samples = pWrapLogger->samples.data();
  pWrapLogger->events.max = pWrapLogger->maxCount;
  pWrapLogger->events.count = 0;

  pWrapLogger->logger = AL_DefaultLogger_Init(AL_GetDefaultAllocator(), timer, &pWrapLogger->events);
}

Logger::~Logger()
{
  AL_ILogger_Deinit(pWrapLogger->logger);

  if(pWrapLogger->outputFile.empty())
    return;

  ofstream tracer(pWrapLogger->outputFile);

  for(int32_t i = 0; i < pWrapLogger->events.count; i++)
    tracer << string(pWrapLogger->samples[i].label) << " " << pWrapLogger->samples[i].timestamp << endl;
}

AL_ILogger* Logger::GetLogger()
{
  return pWrapLogger->logger;
}
