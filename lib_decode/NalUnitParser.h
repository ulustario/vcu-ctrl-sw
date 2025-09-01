// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "I_DecoderCtx.h"

uint32_t GetNonVclSize(AL_TCircBuffer* pBufStream);
void UpdateContextAtEndOfFrame(AL_TDecCtx* pCtx);

/*!@}*/
