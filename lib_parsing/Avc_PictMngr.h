// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "ItuPictMngr.h"
#include "DPB.h"
#include "lib_common/AvcHeaders.h"
#include "lib_common_dec/DecPicParam.h"

/*****************************************************************************
   \brief Sets the POC of the current decoded frame
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] pSlice slice header of the current decoded slice
*****************************************************************************/
void AL_AVC_Dpb_SetCurrentPOC(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice);

/*****************************************************************************
   \brief Sets the picture structure of the current decoded frame
   \param[in] pCtx       Pointer to a Picture manager context object
   \param[in] ePicStruct Picture structure
*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct);

/*****************************************************************************
   \brief This function updates the reconstructed resolution information
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] pCropInfo Pointer to Cropping information
   \param[in] ePicStruct Picture structure (frame/field, top/Bottom) of the current frame buffer
*****************************************************************************/
void AL_AVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCropInfo, AL_EPicStruct ePicStruct);

/*****************************************************************************
   \brief This function updates the DPB context each time a picture have been parsed.
   \param[in] pDpb            Pointer to a DPB context object
*****************************************************************************/
void AL_AVC_Dpb_EndParsing(AL_TDpb* pDpb);

/*****************************************************************************
   \brief Retrieves all buffers (input and output) required to decode the current slice
   \param[in]  pCtx          Pointer to a Picture manager context object
   \param[in]  pPicParam           Pointer to the current picture parameters
   \param[in]  pSliceParam           Pointer to the current slice parameters
   \param[out] pPicBuffers  Pointer to the buffers to be filled
   \param[out] pRecs         Receives pointer to the frame buffers where reconstructed pictures should be stored.
   \return If the function succeeds the return value is nonzero (true)
        If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_AVC_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, AL_TDecBuffers* pPicBuffers);

/*****************************************************************************
   \brief Initializes the reference picture list for the current slice
   \param[in]  pDpb     Pointer to a DPB context object
   \param[in]  pSlice   Current slice header
   \param[out] pListRef Receives the reference list of the current slice
*****************************************************************************/
void AL_AVC_Dpb_InitPictList(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef);

/*****************************************************************************
   \brief Initializes fill Gap in Frame num
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Current slice header
*****************************************************************************/
void AL_AVC_PictMngr_Fill_Gap_In_FrameNum(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice);

/*****************************************************************************
   \brief Reorders the reference picture list of the current slice
   \param[in]     pDpb     Pointer to a DPB context object
   \param[in]     pSlice   Current slice header
   \param[in,out] pListRef Receives the modified reference list of the current slice
*****************************************************************************/
void AL_AVC_Dpb_ReorderPictList(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef);

/*!@}*/
