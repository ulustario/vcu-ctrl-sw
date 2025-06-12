// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

struct AL_TInternalHandleMetaData
{
  AL_HANDLE pHandles;
  AL_MUTEX mutex;
  int32_t numHandles;
  int32_t handleSizeInBytes;
  int32_t maxHandles;
};
