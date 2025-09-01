// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Decoder_Settings
   !@{
   \file
 *****************************************************************************/
#pragma once

/*****************************************************************************
   \brief Decoder DPB mode enum
*****************************************************************************/
typedef enum AL_EDpbMode
{
  AL_DPB_NORMAL, /*!< Follow DPB specification */
  AL_DPB_NO_REORDERING, /*!< Assume there is no reordering in the stream */
  AL_DPB_MAX_ENUM, /*!< sentinel */
}AL_EDpbMode;

/*!@}*/
