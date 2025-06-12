// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Profiles.h"
#include "lib_common/VideoMode.h"

/*****************************************************************************
   \brief Stream's settings
 *************************************************************************/
typedef struct AL_TStreamSettings
{
  AL_TDimension tDim; /*!< Stream's dimension (width / height) */
  AL_EChromaMode eChroma; /*!< Stream's chroma mode (400/420/422/444) */
  int32_t iBitDepth; /*!< Stream's bit depth */
  int32_t iLevel; /*!< Stream's level */
  AL_EProfile eProfile; /*!< Stream's profile */
  AL_ESequenceMode eSequenceMode; /*!< Stream's sequence mode */
  bool bDecodeIntraOnly;  /*!< Should the decoder process only I frames  */
  int32_t iMaxRef; /*!< Stream's max reference frame, 0 if not used*/
}AL_TStreamSettings;
