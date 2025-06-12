// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_common/Planes.h"
}

std::vector<AL_TPlaneDescription> getPlaneDescription(TFourCC tFourCC, int32_t iPitch, int32_t iPitchMap, size_t sizes[], int& iTotalOffset);
