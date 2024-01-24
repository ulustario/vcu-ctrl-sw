// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

struct AL_TInternalHandleMetaData
{
  AL_HANDLE pHandles;
  AL_MUTEX mutex;
  int numHandles;
  int handleSizeInBytes;
  int maxHandles;
};
