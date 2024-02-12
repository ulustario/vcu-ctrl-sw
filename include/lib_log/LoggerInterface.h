// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#define MAX_LOG_LABEL_SIZE 32

typedef struct AL_ILogger AL_ILogger;
typedef struct
{
  void (* pfnLog)(AL_ILogger* logger, char const label[MAX_LOG_LABEL_SIZE]);
  void (* pfnDeinit)(AL_ILogger* logger);
}AL_ILoggerVtable;

struct AL_ILogger
{
  AL_ILoggerVtable const* vtable;
};

static inline void AL_ILogger_Log(AL_ILogger* logger, char const label[MAX_LOG_LABEL_SIZE])
{
  if(logger && logger->vtable)
    logger->vtable->pfnLog(logger, label);
}

static inline void AL_ILogger_Deinit(AL_ILogger* logger)
{
  if(logger && logger->vtable)
    logger->vtable->pfnDeinit(logger);
}
