// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

typedef struct AL_ITimer AL_ITimer;
typedef struct AL_TTimerVTable
{
  AL_64U (* pfnGetTime)(AL_ITimer* timer);
  void (* pfnDeinit)(AL_ITimer* timer);
}AL_TTimerVTable;

struct AL_ITimer
{
  AL_TTimerVTable const* vtable;
};

static inline AL_64U AL_ITimer_GetTime(AL_ITimer* timer)
{
  if(timer)
    return timer->vtable->pfnGetTime(timer);
  return 0;
}

static inline void AL_ITimer_Deinit(AL_ITimer* timer)
{
  if(timer)
    timer->vtable->pfnDeinit(timer);
}
