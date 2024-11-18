// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_base
   !@{
   \file
 *****************************************************************************/
#pragma once

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
  int PictWidth;  /*!< Frame width in pixels */
  int PictHeight; /*!< Frame height in pixels */

  TFourCC FourCC; /*!< FOURCC identifying the file format */

  unsigned int FrameRate;  /*!< Frame by second */
};

/*!@}*/
