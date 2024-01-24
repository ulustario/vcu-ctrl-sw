// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

typedef struct AL_ITimer AL_ITimer;
typedef struct
{
  AL_64U (* pfnGetTime)(AL_ITimer const* timer);
}AL_ITimerVtable;

struct AL_ITimer
{
  AL_ITimerVtable const* vtable;
};

static inline AL_64U AL_ITimer_GetTime(AL_ITimer const* timer)
{
  return timer->vtable->pfnGetTime(timer);
}
