// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common_enc/EncChanParam.h"

/*****************************************************************************
   \brief Get the maximum number of reference buffer required for the encoding parameters
   provided
   \param[in] pChParam Pointer to the channel parameters
   \return The maximum size of the DPB
*****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef(const AL_TEncChanParam* pChParam);

/*****************************************************************************
   \brief Get the maximum size of the dpb required for the encoding parameters
   provided
   \param[in] pChParam Pointer to the channel parameters
   \return The maximum size of the DPB
*****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam);

/*****************************************************************************
   \brief Get the maximum picture reordering required by the gop pattern
   provided. It is the maximum number of frames that can precede one frame in
   encoding order and follow that frame in display order.
   \param[in] pChParam Pointer to the channel parameters
   \return The maximum picture reordering
*****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxReordering(const AL_TEncChanParam* pChParam);
