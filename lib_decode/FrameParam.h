// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "I_DecoderCtx.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecPicParam.h"

#include "lib_parsing/DPB.h"

#include "lib_common/AvcHeaders.h"
/******************************************************************************/
void AL_AVC_FillSliceParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam, AL_TDecPicParam* pPicParam, bool bConceal);
void AL_AVC_FillPictParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam);
void AL_AVC_FillSlicePicIdRegister(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSliceParam);
int32_t AL_AVC_GetFrameHeight(AL_TAvcSps const* pSPS, bool bHasFields);
AL_EPicStruct AL_AVC_GetPicStruct(AL_TAvcSliceHdr const* pSlice);

#include "lib_common/HevcHeaders.h"
/******************************************************************************/
void AL_HEVC_FillSliceParameters(AL_THevcSliceHdr const* pSlice, AL_TDecCtx const* pCtx, AL_TDecSliceParam* pSliceParam);
void AL_HEVC_FillPictParameters(AL_THevcSliceHdr const* pSlice, AL_TDecCtx const* pCtx, AL_TDecPicParam* pPicParam);
void AL_HEVC_FillSlicePicIdRegister(AL_THevcSliceHdr const* pSlice, AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam);

/*!@}*/
