// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
/******************************************************************************
   \addtogroup lib_app
   !@{
   \file
 *****************************************************************************/

#include <vector>
extern "C"
{
#include "lib_common/FourCC.h"
}

/*****************************************************************************
   \brief YUV File size and format information
*****************************************************************************/
AL_INTROSPECT(category = "debug") struct AL_TYUVFileInfo
{
  int32_t PictWidth;  /*!< Frame width in pixels */
  int32_t PictHeight; /*!< Frame height in pixels */

  TFourCC FourCC; /*!< FOURCC identifying the file format */

  unsigned int FrameRate;  /*!< Frame by second */
};

/*!@}*/
