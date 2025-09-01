// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "I_PictMngr.h"
#include "lib_common_dec/DecPicParam.h"

typedef enum
{
  ITU_ANNEX_BUF_MV_IDX,
  ITU_ANNEX_BUF_POC_IDX,
  ITU_ANNEX_NUM_BUF,
}EItuAnnexBuf;

bool AL_ItuPictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_ECodec eCodec, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, AL_TDecBuffers* pPicBuffers);
