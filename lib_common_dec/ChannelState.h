// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

typedef enum AL_e_ChanState
{
  CHAN_UNINITIALIZED,
  CHAN_CONFIGURED,
  CHAN_INVALID,
  CHAN_DESTROYING,
}AL_EChanState;
