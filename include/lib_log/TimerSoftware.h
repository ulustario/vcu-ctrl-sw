// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma  once

#include "lib_log/TimerInterface.h"
#include "lib_common/Allocator.h"

AL_ITimer* AL_SoftwareTimer_Init(char const* name, AL_TAllocator* allocator);
