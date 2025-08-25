// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_log/LoggerInterface.h"
#include "lib_log/TimerInterface.h"
#include "lib_common/Allocator.h"

AL_ILogger* AL_RtosLogger_Init(AL_TAllocator* allocator, AL_ITimer* timer);
