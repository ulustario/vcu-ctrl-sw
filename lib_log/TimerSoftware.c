// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_log/TimerSoftware.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

typedef struct
{
  AL_TTimerVTable const* vtable;
  AL_HANDLE handle;
  AL_TAllocator* allocator;
}AL_SoftwareTimer;

static AL_64U getSoftwareTime(AL_ITimer* timer)
{
  (void)timer;
  return Rtos_GetTime();
}

static void deinit(AL_ITimer* timer)
{
  if(NULL == timer)
    return;

  AL_Allocator_Free(((AL_SoftwareTimer*)timer)->allocator, ((AL_SoftwareTimer*)timer)->handle);
}

static AL_TTimerVTable const SoftwareTimerVtable =
{
  &getSoftwareTime,
  &deinit,
};

AL_ITimer* AL_SoftwareTimer_Init(char const* name, AL_TAllocator* allocator)
{
  if(allocator == NULL)
    return NULL;

  AL_HANDLE handle = AL_Allocator_AllocNamed(allocator, sizeof(AL_SoftwareTimer), name);

  if(handle == NULL)
    return NULL;

  AL_SoftwareTimer* timer = (AL_SoftwareTimer*)AL_Allocator_GetVirtualAddr(allocator, handle);

  timer->vtable = &SoftwareTimerVtable;
  timer->handle = handle;
  timer->allocator = allocator;

  return (AL_ITimer*)timer;
}
