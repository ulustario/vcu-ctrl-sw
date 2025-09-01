// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#include "Avc_PictMngr.h"
#include "AvcParser.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferPixMapMeta.h"
#include "lib_common/AvcUtils.h"
#include "lib_rtos/types.h"

/*****************************************************************************/
static void AL_sGetPocType0(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  int32_t iPrevPocMSB = 0;
  int32_t iPrevPocLSB = 0;
  int32_t iMaxPocLSB = 1L << (pSlice->pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4);

  if(!AL_AVC_IsIDR(pSlice->nal_unit_type))
  {
    if(AL_Dpb_LastHasMMCO5(pDpb))
    {
      /*warning : work in frame only*/
      iPrevPocLSB = pDpb->iTopFieldOrderCnt;
    }
    else
    {
      iPrevPocMSB = pDpb->iPrevPocMSB;
      iPrevPocLSB = pDpb->iPrevPocLSB;
    }
  }

  if((pSlice->pic_order_cnt_lsb < iPrevPocLSB) &&
     ((iPrevPocLSB - pSlice->pic_order_cnt_lsb) >= (iMaxPocLSB / 2)))
    iPrevPocMSB = iPrevPocMSB + iMaxPocLSB;

  else if((pSlice->pic_order_cnt_lsb > iPrevPocLSB) &&
          ((pSlice->pic_order_cnt_lsb - iPrevPocLSB) > (iMaxPocLSB / 2)))
    iPrevPocMSB = iPrevPocMSB - iMaxPocLSB;

  /*warning : work in frame only*/
  pDpb->iTopFieldOrderCnt = iPrevPocMSB + pSlice->pic_order_cnt_lsb;
  pDpb->iBotFieldOrderCnt = pDpb->iTopFieldOrderCnt + pSlice->delta_pic_order_cnt_bottom;

  if(pSlice->nal_ref_idc)
  {
    pDpb->iPrevPocLSB = pSlice->pic_order_cnt_lsb;
    pDpb->iPrevPocMSB = iPrevPocMSB;
  }
}

/*****************************************************************************/
static void AL_sGetPocType1(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  AL_64S iExpectedDeltaPerPicOrderCntCycle = 0;
  bool bIsIDR = AL_AVC_IsIDR(pSlice->nal_unit_type);

  for(AL_64S i = 0; i < pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle; ++i)
    iExpectedDeltaPerPicOrderCntCycle += pSlice->pSPS->offset_for_ref_frame[i];

  if(!bIsIDR)
  {
    if(AL_Dpb_LastHasMMCO5(pDpb))
    {
      pDpb->iPrevFrameNumOffset = 0;
      pDpb->iPrevFrameNum = 0;
    }
  }

  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  AL_64S iFrameNumOffset;

  if(bIsIDR)
    iFrameNumOffset = 0;
  else if(pDpb->iPrevFrameNum > pSlice->frame_num)
    iFrameNumOffset = pDpb->iPrevFrameNumOffset + iMaxFrameNum;
  else
    iFrameNumOffset = pDpb->iPrevFrameNumOffset;

  AL_64S iAbsFrameNum = 0;

  if(pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle)
    iAbsFrameNum = iFrameNumOffset + pSlice->frame_num;

  if(!pSlice->nal_ref_idc && iAbsFrameNum > 0)
    iAbsFrameNum -= 1;

  AL_64S iExpectedPicOrderCnt = 0;

  if(iAbsFrameNum > 0)
  {
    AL_64S iPicOrderCntCycleCnt = (iAbsFrameNum - 1) / pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle;
    AL_64S iFrameNumInPicOrderCntCycle = (iAbsFrameNum - 1) % pSlice->pSPS->num_ref_frames_in_pic_order_cnt_cycle;
    iExpectedPicOrderCnt = iPicOrderCntCycleCnt * iExpectedDeltaPerPicOrderCntCycle;

    for(AL_64S i = 0; i <= iFrameNumInPicOrderCntCycle; ++i)
      iExpectedPicOrderCnt += pSlice->pSPS->offset_for_ref_frame[i];
  }

  if(!pSlice->nal_ref_idc)
    iExpectedPicOrderCnt += pSlice->pSPS->offset_for_non_ref_pic;

  pDpb->iPrevFrameNumOffset = iFrameNumOffset;
  /*warning : work only in frame mode*/
  pDpb->iTopFieldOrderCnt = iExpectedPicOrderCnt + pSlice->delta_pic_order_cnt[0];
  pDpb->iBotFieldOrderCnt = pDpb->iTopFieldOrderCnt + pSlice->pSPS->offset_for_top_to_bottom_field +
                            pSlice->delta_pic_order_cnt[1];
}

/*****************************************************************************/
static void AL_sGetPocType2(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  bool bIsIDR = AL_AVC_IsIDR(pSlice->nal_unit_type);

  if(!bIsIDR)
  {
    if(AL_Dpb_LastHasMMCO5(pDpb))
    {
      pDpb->iPrevFrameNumOffset = 0;
      pDpb->iPrevFrameNum = 0;
    }
  }

  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  AL_64S iFrameNumOffset;

  if(bIsIDR)
    iFrameNumOffset = 0;
  else if(pDpb->iPrevFrameNum > pSlice->frame_num)
    iFrameNumOffset = pDpb->iPrevFrameNumOffset + iMaxFrameNum;
  else
    iFrameNumOffset = pDpb->iPrevFrameNumOffset;

  AL_64S iTempPicOrderCnt;

  if(bIsIDR)
    iTempPicOrderCnt = 0;
  else if(!pSlice->nal_ref_idc)
    iTempPicOrderCnt = 2 * (iFrameNumOffset + pSlice->frame_num) - 1;
  else
    iTempPicOrderCnt = 2 * (iFrameNumOffset + pSlice->frame_num);

  pDpb->iPrevFrameNumOffset = iFrameNumOffset;
  /*warning : work only in frame mode*/
  pDpb->iTopFieldOrderCnt = iTempPicOrderCnt;
  pDpb->iBotFieldOrderCnt = iTempPicOrderCnt;
}

/*****************************************************************************/
static int32_t AL_sCalculatePOC(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  switch(pSlice->pSPS->pic_order_cnt_type)
  {
  case 0:
    AL_sGetPocType0(pDpb, pSlice);
    break;

  case 1:
    AL_sGetPocType1(pDpb, pSlice);
    break;

  case 2:
    AL_sGetPocType2(pDpb, pSlice);
    break;

  default:
    return 0xBAADF00D;
  }

  return (pDpb->iTopFieldOrderCnt < pDpb->iBotFieldOrderCnt) ? pDpb->iTopFieldOrderCnt :
         pDpb->iBotFieldOrderCnt;
}

/*****************************************************************************/
void AL_AVC_Dpb_SetCurrentPOC(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice)
{
  pDpb->iCurFramePOC = AL_sCalculatePOC(pDpb, pSlice);
}

/*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;
  pDpb->ePicStruct = ePicStruct;
}

/*****************************************************************************/
void AL_AVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCropInfo, AL_EPicStruct ePicStruct)
{
  AL_PictMngr_UpdateDisplayBufferCrop(pCtx, pCropInfo);
  AL_PictMngr_UpdateDisplayBufferPicStruct(pCtx, ePicStruct);
}

/***************************************************************************/
void AL_AVC_Dpb_EndParsing(AL_TDpb* pDpb)
{
  // increment present pictures latency count
  AL_TIndex tNodeID = AL_Dpb_GetHeadPOC(pDpb);

  while(IS_NODE_VALID(tNodeID))
  {
    AL_Dpb_IncrementPicLatency(pDpb, tNodeID);
    tNodeID = AL_Dpb_GetNextPOC(pDpb, tNodeID);
  }

  AL_TIndex tDeleteNodeID = AL_Dpb_SearchPOC(pDpb, pDpb->iCurFramePOC);
  bool bIsPOCAlreadyInDPB = IS_NODE_VALID(tDeleteNodeID);

  if(bIsPOCAlreadyInDPB)
  {
    if(AL_Dpb_GetOutputFlag(pDpb, tDeleteNodeID))
      AL_Dpb_Display(pDpb, tDeleteNodeID);

    AL_Dpb_Remove(pDpb, tDeleteNodeID);
  }

  AL_Dpb_ResetMMCO5(pDpb);
}

/***************************************************************************/
bool AL_AVC_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, AL_TDecBuffers* pPicBuffers)
{
  if(!AL_ItuPictMngr_GetBuffers(pCtx, AL_CODEC_AVC, pSliceParam, pRecs, pPicBuffers))
    return false;

  return true;
}

/*****************************************************************************/
void AL_AVC_PictMngr_Fill_Gap_In_FrameNum(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice)
{
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;
  int32_t iMaxFrameNum = 1 << (pSlice->pSPS->log2_max_frame_num_minus4 + 4);

  if(!(
       (!AL_AVC_IsIDR(pSlice->nal_unit_type))
       && (pSlice->frame_num != pDpb->iPrevFrameNum)
       && (pSlice->frame_num != ((pDpb->iPrevFrameNum + 1) % iMaxFrameNum))
       ))
    return;

  if(pSlice->pSPS->gaps_in_frame_num_value_allowed_flag == 0)
    return;

  int32_t iUnusedShortTermFrameNum = (pDpb->iPrevFrameNum + 1) % iMaxFrameNum;
  int32_t iCurrFrameNum = pSlice->frame_num;

  while(iCurrFrameNum != iUnusedShortTermFrameNum)
  {
    AL_TAvcSliceHdr pUnusedSlice = *pSlice;

    pUnusedSlice.frame_num = iUnusedShortTermFrameNum;
    pUnusedSlice.adaptive_ref_pic_marking_mode_flag = 0;

    AL_TDpbInsertParam tParam;
    Rtos_Memset(&tParam, 0, sizeof(AL_TDpbInsertParam));
    tParam.iFramePOC = AL_sCalculatePOC(pDpb, &pUnusedSlice);
    tParam.ePicStruct = AL_PS_FRM;
    tParam.eMarkingFlag = SHORT_TERM_REF;
    tParam.bNonExisting = true;
    AL_PictMngr_Insert(pCtx, AL_BAD_INDEX, AL_BAD_INDEX, &tParam);

    AL_Dpb_MarkingProcess(pDpb, &pUnusedSlice, pDpb->iCurFramePOC);
    AL_Dpb_AVC_Cleanup(pDpb);
    pDpb->iPrevFrameNum = iUnusedShortTermFrameNum;
    iUnusedShortTermFrameNum = (iUnusedShortTermFrameNum + 1) % iMaxFrameNum;
  }
}

/*****************************************************************************/
void AL_AVC_Dpb_InitPictList(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  for(uint8_t uRef = 0; uRef < AL_MAX_REF; ++uRef)
  {
    (*pListRef)[0][uRef].tNodeID = AL_BAD_INDEX;
    (*pListRef)[1][uRef].tNodeID = AL_BAD_INDEX;
  }

  if(pSlice->slice_type == AL_SLICE_P || pSlice->slice_type == AL_SLICE_SP)
    AL_Dpb_InitPSlice_RefList(pDpb, pDpb->ePicStruct, &(*pListRef)[0][0]);
  else if(pSlice->slice_type == AL_SLICE_B)
    AL_Dpb_InitBSlice_RefList(pDpb, pDpb->iCurFramePOC, pDpb->ePicStruct, pListRef);
}

/*****************************************************************************/
void AL_AVC_Dpb_ReorderPictList(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef)
{
  int32_t iPicNumPred = pSlice->frame_num * (1 + pSlice->field_pic_flag) + pSlice->field_pic_flag;

  if(pSlice->ref_pic_list_reordering_flag_l0)
  {
    uint8_t uParse = 0;
    uint8_t uRefIdxL0 = 0;
    uint8_t uParseShort = 0;
    uint8_t uParseLong = 0;

    while(uParse < AL_AVC_MAX_REFERENCE_PICTURE_REORDER && pSlice->reordering_of_pic_nums_idc_l0[uParse] != 3)
    {
      int32_t iPicNumIdc = pSlice->reordering_of_pic_nums_idc_l0[uParse++];
      switch(iPicNumIdc)
      {
      case 0:
      case 1:
        AL_Dpb_ModifShortTerm(pDpb, pSlice, iPicNumIdc, uParseShort++, 0, &uRefIdxL0, &iPicNumPred, pListRef);
        break;
      case 2:
        AL_Dpb_ModifLongTerm(pDpb, pSlice, uParseLong++, 0, &uRefIdxL0, pListRef);
        break;
      default:
        break;
      }
    }
  }

  if(pSlice->ref_pic_list_reordering_flag_l1 && pSlice->slice_type == AL_SLICE_B)
  {
    uint8_t uParse = 0;
    uint8_t uRefIdxL1 = 0;
    uint8_t uParseShort = 0;
    uint8_t uParseLong = 0;

    iPicNumPred = pSlice->frame_num * (1 + pSlice->field_pic_flag) + pSlice->field_pic_flag;

    while(uParse < AL_AVC_MAX_REFERENCE_PICTURE_REORDER && pSlice->reordering_of_pic_nums_idc_l1[uParse] != 3)
    {
      int32_t iPicNumIdc = pSlice->reordering_of_pic_nums_idc_l1[uParse++];
      switch(iPicNumIdc)
      {
      case 0:
      case 1:
        AL_Dpb_ModifShortTerm(pDpb, pSlice, iPicNumIdc, uParseShort++, 1, &uRefIdxL1, &iPicNumPred, pListRef);
        break;
      case 2:
        AL_Dpb_ModifLongTerm(pDpb, pSlice, uParseLong++, 1, &uRefIdxL1, pListRef);
        break;
      default:
        break;
      }
    }
  }
}

/*!@}*/
