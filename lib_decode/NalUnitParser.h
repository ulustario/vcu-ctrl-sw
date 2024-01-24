// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_parsing/AvcParser.h"
#include "lib_parsing/HevcParser.h"

typedef struct t_Dec_Ctx AL_TDecCtx;

uint32_t GetNonVclSize(TCircBuffer* pBufStream);
void UpdateContextAtEndOfFrame(AL_TDecCtx* pCtx);

/*@}*/
