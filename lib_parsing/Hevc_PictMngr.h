// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "I_PictMngr.h"
#include "lib_common_dec/DecPicParam.h"
#include "lib_common/HevcHeaders.h"
#include "lib_common/Nuts.h"
#include "DPB.h"

/*****************************************************************************
   \brief This function updates the reconstructed resolution information
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] pCropInfo Pointer to Cropping information
   \param[in] ePicStruct Picture structure (frame/field, top/Bottom) of the current frame buffer
*****************************************************************************/
void AL_HEVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCropInfo, AL_EPicStruct ePicStruct);

/*****************************************************************************
   \brief Remove from the DPB all unused pictures(non-reference and not needed for output
   \param[in] pCtx           Pointer to a Picture manager context object
   \param[in] pSPS           Pointer to the Sequence Parameter Set structure holding info on picture dpb latency
   \param[in] bClearRef      Specifies if the reference pool picture is cleared
   \param[in] bNoOutputPrior Specifies if the pictures still stored in the DPB will whether be output or discarded when bClearRef = true
*****************************************************************************/
void AL_HEVC_PictMngr_ClearDPB(AL_TPictMngrCtx* pCtx, AL_THevcSps const* pSPS, bool bClearRef, bool bNoOutputPrior);

/*****************************************************************************
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx            Pointer to a Picture manager context object
   \param[in] uPocLsb         Value used to identify long term reference picture
   \param[in] eNUT            NalUnitType of the current decoded picture
   \param[in] pSlice          Pointer to the last slice header current's frame
   \param[in] bPicOutputFlag Specifies whether the current picture is displayed or not
*****************************************************************************/
void AL_HEVC_PictMngr_EndFrame(AL_TPictMngrCtx* pCtx, uint32_t uPocLsb, AL_ENut eNUT, AL_THevcSliceHdr const* pSlice, bool bPicOutputFlag);

/*****************************************************************************
   \brief This function remove from the DPB the oldest picture if it is full.
   \param[in] pDpb            Pointer to a DPB context object
*****************************************************************************/
void AL_HEVC_Dpb_RemoveHeadFrame(AL_TDpb* pDpb);

/*****************************************************************************
   \brief This function return true if the DPB has reference..
   \param[in] pCtx   Pointer to a Picture manager context object
*****************************************************************************/
bool AL_HEVC_Dpb_HasPictInDPB(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief Prepares the reference picture set for the current slice reference picture list construction
   \param[in]  pDpb       Pointer to a DPB context object
   \param[in]  pSlice     Pointer to the slice header of the current slice
*****************************************************************************/
void AL_HEVC_Dpb_InitRefPictSet(AL_TDpb* pDpb, AL_THevcSliceHdr const* pSlice);

/*****************************************************************************
   \brief Builds the reference picture list of the current slice
   \param[in]  pDpb     Pointer to a DPB context object
   \param[in]  pSlice   Pointer to the slice header of the current slice
   \param[out] pListRef Pointer to the current reference list
*****************************************************************************/
bool AL_HEVC_Dpb_BuildPictureList(AL_TDpb* pDpb, AL_THevcSliceHdr const* pSlice, TBufferListRef* pListRef);

/*!@}*/
