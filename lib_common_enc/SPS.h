// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufConst.h"
#include "lib_common/common_syntax_elements.h"

#include "lib_common/AvcHeaders.h"

#include "lib_common/HevcHeaders.h"

/****************************************************************************/
typedef union
{
  AL_TAvcSps AvcSPS;
  AL_THevcSps HevcSPS;
}AL_TSps;
