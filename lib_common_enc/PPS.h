// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/AvcHeaders.h"

#include "lib_common/HevcHeaders.h"

/****************************************************************************/
typedef union AL_TPps
{
  AL_TAvcPps AvcPPS;
  AL_THevcPps HevcPPS;
}AL_TPps;
