// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/plateform.h"

#include <windows.h>

void InitializePlateform(void)
{
  SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
}
