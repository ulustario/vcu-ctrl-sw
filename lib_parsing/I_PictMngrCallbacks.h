// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common_dec/Types.h"

typedef struct
{
  void (* pfnIncrementFrmBuf)(void* pUserParam, AL_TIndex tFrameID); /*!< Callback Callback Function to signal that a Frame Buffer is used by the reference manager */
  void (* pfnDecrementFrmBuf)(void* pUserParam, AL_TIndex tFrameID); /*!< Callback Callback Function to signal that a frame buffer is no more used by the reference manager */
  void (* pfnOutputFrmBuf)(void* pUserParam, AL_TIndex tFrameID); /*!< Callback Function to signal that a frame buffer need to be displayed */

  void (* pfnIncrementAnnexBuf)(void* pUserParam, AL_TIndex tID); /*!< Callback Callback Function to signal that a annex buffer is used by the reference manager */
  void (* pfnDecrementAnnexBuf)(void* pUserParam, AL_TIndex tID); /*!< Callback Callback Function to signal that a annex buffer is no more used by the reference manager */

  void* pUserParam; /*!< pointer to be passed each time one to the following callback is called */
}AL_TPictMngrCallbacks;
