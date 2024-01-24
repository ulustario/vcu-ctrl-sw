// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma  once

#include "lib_log/TimerInterface.h"
typedef struct
{
  AL_ITimerVtable const* vtable;
}AL_SoftwareTimer;
AL_ITimer* AL_SoftwareTimerInit(AL_SoftwareTimer* timer);

extern AL_SoftwareTimer g_SoftwareTimer;
