// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "HevcParser.h"
#include "lib_common/Utils.h"
#include "lib_common/SeiInternal.h"
#include "lib_common/ScalingList.h"
#include "SeiParser.h"

#define CONCEAL_LEVEL_IDC 60 * 3

/*****************************************************************************/
static void initPps(AL_THevcPps* pPPS)
{
  Rtos_Memset(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag, 0, sizeof(pPPS->scaling_list_param.UseDefaultScalingMatrixFlag));
  Rtos_Memset(pPPS->cb_qp_offset_list, 0, sizeof(pPPS->cb_qp_offset_list));
  Rtos_Memset(pPPS->cr_qp_offset_list, 0, sizeof(pPPS->cr_qp_offset_list));

  pPPS->loop_filter_across_tiles_enabled_flag = 1;
  pPPS->lists_modification_present_flag = 1;
  pPPS->num_tile_columns_minus1 = 0;
  pPPS->num_tile_rows_minus1 = 0;
  pPPS->deblocking_filter_override_enabled_flag = 0;
  pPPS->pps_deblocking_filter_disabled_flag = 0;
  pPPS->pps_beta_offset_div2 = 0;
  pPPS->pps_tc_offset_div2 = 0;

  pPPS->pps_extension_7bits = 0;
  pPPS->pps_range_extension_flag = 0;
  pPPS->log2_transform_skip_block_size_minus2 = 0;
  pPPS->cross_component_prediction_enabled_flag = 0;
  pPPS->diff_cu_chroma_qp_offset_depth = 0;
  pPPS->chroma_qp_offset_list_enabled_flag = 0;
  pPPS->log2_sao_offset_scale_luma = 0;
  pPPS->log2_sao_offset_scale_chroma = 0;

  pPPS->bConceal = true;
}

/*****************************************************************************/
static bool hevc_is_default_dc_coeff(uint8_t uSizeID)
{
  return uSizeID > 1;
}

/*****************************************************************************/
static void hevc_scaling_list_data(AL_TSCLParam* pSCLParam, AL_TRbspParser* pRP)
{
  for(uint8_t uSizeID = 0; uSizeID < 4; ++uSizeID)
  {
    uint8_t uIncr = (uSizeID == 3) ? 3 : 1;

    for(uint8_t uMatrixID = 0; uMatrixID < 6; uMatrixID += uIncr)
    {
      pSCLParam->scaling_list_pred_mode_flag[uSizeID][uMatrixID] = u(pRP, 1);

      if(!pSCLParam->scaling_list_pred_mode_flag[uSizeID][uMatrixID])
      {
        pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID] = ue(pRP);

        if(!pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID])
        {
          if(hevc_is_default_dc_coeff(uSizeID))
            pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = 16;

          if(uSizeID) /* superior to 4x4 */
            Rtos_Memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], AL_HEVC_DefaultScalingLists8x8[uMatrixID / 3], 64);
          else /* equal to 4x4 */
            Rtos_Memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], AL_HEVC_DefaultScalingLists4x4[uMatrixID / 3], 16);
        }
        else
        {
          uint8_t uPredMatrixID = Clip3(uMatrixID - pSCLParam->scaling_list_pred_matrix_id_delta[uSizeID][uMatrixID], 0, ((uSizeID == 3) ? 0 : 4));

          if(hevc_is_default_dc_coeff(uSizeID))
            pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uPredMatrixID];

          if(uSizeID) /* superior to 4x4 */
            Rtos_Memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], pSCLParam->ScalingList[uSizeID][uPredMatrixID], 64);
          else /* equal to 4x4 */
            Rtos_Memcpy(pSCLParam->ScalingList[uSizeID][uMatrixID], pSCLParam->ScalingList[uSizeID][uPredMatrixID], 16);
        }
      }
      else
      {
        int16_t uNextCoeff = 8;
        uint16_t uCoeffNum = Min(64, (1 << (4 + (uSizeID << 1))));
        uint8_t const* pScanOrder = (uCoeffNum == 64) ? AL_HEVC_ScanOrder8x8 : AL_HEVC_ScanOrder4x4;

        if(hevc_is_default_dc_coeff(uSizeID))
        {
          uNextCoeff = Clip3(se(pRP), -7, 247) + 8;
          pSCLParam->scaling_list_dc_coeff[uSizeID - 2][uMatrixID] = uNextCoeff;
        }

        for(uint8_t uCoeff = 0; uCoeff < uCoeffNum; ++uCoeff)
        {
          uNextCoeff = (uNextCoeff + Clip3(se(pRP), -128, 127) + 256) % 256; // scaling_list_delta_coeff
          pSCLParam->ScalingList[uSizeID][uMatrixID][pScanOrder[uCoeff]] = uNextCoeff;
        }
      }
    }
  }
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId)
{
  skipAllZerosAndTheNextByte(pRP);
  u(pRP, 16); // Skip NUT + temporal_id

  uint16_t pps_id = ue(pRP);

  if(pPpsId)
    *pPpsId = pps_id;

  COMPLY(pps_id < AL_HEVC_MAX_PPS);

  AL_THevcAup* aup = &pIAup->hevcAup;
  AL_THevcPps* pPPS = &aup->pPPS[pps_id];

  // default values
  initPps(pPPS);

  pPPS->pps_pic_parameter_set_id = pps_id;
  pPPS->pps_seq_parameter_set_id = ue(pRP);

  COMPLY(pPPS->pps_seq_parameter_set_id < AL_HEVC_MAX_SPS);

  pPPS->pSPS = &aup->pSPS[pPPS->pps_seq_parameter_set_id];

  COMPLY(!pPPS->pSPS->bConceal);

  uint16_t uLCUPicWidth = pPPS->pSPS->PicWidthInCtbs;
  uint16_t uLCUPicHeight = pPPS->pSPS->PicHeightInCtbs;

  pPPS->dependent_slice_segments_enabled_flag = u(pRP, 1);
  pPPS->output_flag_present_flag = u(pRP, 1);
  pPPS->num_extra_slice_header_bits = u(pRP, 3);
  pPPS->sign_data_hiding_flag = u(pRP, 1);
  pPPS->cabac_init_present_flag = u(pRP, 1);

  pPPS->num_ref_idx_l0_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);
  pPPS->num_ref_idx_l1_default_active_minus1 = Clip3(ue(pRP), 0, AL_HEVC_MAX_REF_IDX);

  uint16_t QpBdOffset = 6 * pPPS->pSPS->bit_depth_luma_minus8;
  pPPS->init_qp_minus26 = Clip3(se(pRP), -(26 + QpBdOffset), AL_MAX_INIT_QP);

  pPPS->constrained_intra_pred_flag = u(pRP, 1);
  pPPS->transform_skip_enabled_flag = u(pRP, 1);

  pPPS->cu_qp_delta_enabled_flag = u(pRP, 1);

  if(pPPS->cu_qp_delta_enabled_flag)
    pPPS->diff_cu_qp_delta_depth = Clip3(ue(pRP), 0, pPPS->pSPS->log2_diff_max_min_luma_coding_block_size);

  pPPS->pps_cb_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_cr_qp_offset = Clip3(se(pRP), AL_MIN_QP_OFFSET, AL_MAX_QP_OFFSET);
  pPPS->pps_slice_chroma_qp_offsets_present_flag = u(pRP, 1);

  pPPS->weighted_pred_flag = u(pRP, 1);
  pPPS->weighted_bipred_flag = u(pRP, 1);

  pPPS->transquant_bypass_enabled_flag = u(pRP, 1);
  pPPS->tiles_enabled_flag = u(pRP, 1);
  pPPS->entropy_coding_sync_enabled_flag = u(pRP, 1);

  if(pPPS->tiles_enabled_flag)
  {
    pPPS->num_tile_columns_minus1 = ue(pRP);
    pPPS->num_tile_rows_minus1 = ue(pRP);
    pPPS->uniform_spacing_flag = u(pRP, 1);

    COMPLY(!(pPPS->num_tile_columns_minus1 >= uLCUPicWidth || pPPS->num_tile_rows_minus1 >= uLCUPicHeight ||
             pPPS->num_tile_columns_minus1 >= AL_MAX_COLUMNS_TILE || pPPS->num_tile_rows_minus1 >= AL_MAX_ROWS_TILE));

    if(!pPPS->uniform_spacing_flag)
    {
      uint16_t uClmnOffset = 0;
      uint16_t uLineOffset = 0;

      for(uint8_t i = 0; i < pPPS->num_tile_columns_minus1; ++i)
      {
        pPPS->pTileColWidths[i] = ue(pRP) + 1;
        uClmnOffset += pPPS->pTileColWidths[i];
      }

      COMPLY(uClmnOffset < uLCUPicWidth);

      for(uint8_t i = 0; i < pPPS->num_tile_rows_minus1; ++i)
      {
        pPPS->pTileRowHeights[i] = ue(pRP) + 1;
        uLineOffset += pPPS->pTileRowHeights[i];
      }

      COMPLY(uLineOffset < uLCUPicHeight);

      pPPS->pTileColWidths[pPPS->num_tile_columns_minus1] = uLCUPicWidth - uClmnOffset;
      pPPS->pTileRowHeights[pPPS->num_tile_rows_minus1] = uLCUPicHeight - uLineOffset;
    }
    else /* tile of same size */
    {
      uint16_t num_clmn = pPPS->num_tile_columns_minus1 + 1;
      uint16_t num_line = pPPS->num_tile_rows_minus1 + 1;

      for(uint8_t i = 0; i <= pPPS->num_tile_columns_minus1; ++i)
        pPPS->pTileColWidths[i] = (((i + 1) * uLCUPicWidth) / num_clmn) - ((i * uLCUPicWidth) / num_clmn);

      for(uint8_t i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
        pPPS->pTileRowHeights[i] = (((i + 1) * uLCUPicHeight) / num_line) - ((i * uLCUPicHeight) / num_line);
    }

    /* register tile topology within the frame */
    for(uint8_t i = 0; i <= pPPS->num_tile_rows_minus1; ++i)
    {
      for(uint8_t j = 0; j <= pPPS->num_tile_columns_minus1; ++j)
      {
        uint8_t line = 0;
        uint8_t clmn = 0;
        uint8_t uClmn = 0;
        uint8_t uLine = 0;

        while(line < i)
          uLine += pPPS->pTileRowHeights[line++];

        while(clmn < j)
          uClmn += pPPS->pTileColWidths[clmn++];

        pPPS->TileTopology[(i * (pPPS->num_tile_columns_minus1 + 1)) + j] = uLine * uLCUPicWidth + uClmn;
      }
    }

    pPPS->loop_filter_across_tiles_enabled_flag = u(pRP, 1);
  }

  pPPS->loop_filter_across_slices_enabled_flag = u(pRP, 1);
  pPPS->deblocking_filter_control_present_flag = u(pRP, 1);

  if(pPPS->deblocking_filter_control_present_flag)
  {
    pPPS->deblocking_filter_override_enabled_flag = u(pRP, 1);
    pPPS->pps_deblocking_filter_disabled_flag = u(pRP, 1);

    if(!pPPS->pps_deblocking_filter_disabled_flag)
    {
      pPPS->pps_beta_offset_div2 = Clip3(se(pRP), AL_HEVC_MIN_DBF_PARAM, AL_HEVC_MAX_DBF_PARAM);
      pPPS->pps_tc_offset_div2 = Clip3(se(pRP), AL_HEVC_MIN_DBF_PARAM, AL_HEVC_MAX_DBF_PARAM);
    }
  }

  pPPS->pps_scaling_list_data_present_flag = u(pRP, 1);

  if(pPPS->pps_scaling_list_data_present_flag)
    hevc_scaling_list_data(&pPPS->scaling_list_param, pRP);
  else // get scaling_list data from associated sps
    pPPS->scaling_list_param = pPPS->pSPS->scaling_list_param;

  pPPS->lists_modification_present_flag = u(pRP, 1);
  pPPS->log2_parallel_merge_level_minus2 = Clip3(ue(pRP), 0, pPPS->pSPS->Log2CtbSize - 2);

  pPPS->slice_segment_header_extension_present_flag = u(pRP, 1);
  pPPS->pps_extension_present_flag = u(pRP, 1);

  if(pPPS->pps_extension_present_flag)
  {
    pPPS->pps_range_extension_flag = u(pRP, 1);
    pPPS->pps_extension_7bits = u(pRP, 7);
  }

  if(pPPS->pps_range_extension_flag)
  {
    if(pPPS->transform_skip_enabled_flag)
      pPPS->log2_transform_skip_block_size_minus2 = ue(pRP);
    pPPS->cross_component_prediction_enabled_flag = u(pRP, 1);
    pPPS->chroma_qp_offset_list_enabled_flag = u(pRP, 1);

    if(pPPS->chroma_qp_offset_list_enabled_flag)
    {
      pPPS->diff_cu_chroma_qp_offset_depth = ue(pRP);
      pPPS->chroma_qp_offset_list_len_minus1 = Clip3(ue(pRP), 0, 5);

      for(uint8_t u = 0; u <= pPPS->chroma_qp_offset_list_len_minus1; u++)
      {
        pPPS->cb_qp_offset_list[u] = se(pRP);
        pPPS->cr_qp_offset_list[u] = se(pRP);
      }
    }
    pPPS->log2_sao_offset_scale_luma = ue(pRP);
    pPPS->log2_sao_offset_scale_chroma = ue(pRP);
  }

  if(pPPS->pps_extension_7bits) // pps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // pps_extension_data_flag
  }

  COMPLY(rbsp_trailing_bits(pRP));

  return AL_OK;
}

/*****************************************************************************/
static bool AL_HEVC_sComputeRefPicSetVariables(AL_THevcSps* pSPS, uint8_t RpsIdx)
{
  uint8_t num_negative = 0, num_positive = 0;
  AL_TRefPicSet ref_pic_set = pSPS->short_term_ref_pic_set[RpsIdx];

  if(ref_pic_set.inter_ref_pic_set_prediction_flag)
  {
    uint8_t RIdx = RpsIdx - (ref_pic_set.delta_idx_minus1 + 1);
    int32_t DeltaRPS = (1 - (ref_pic_set.delta_rps_sign << 1)) * (ref_pic_set.abs_delta_rps_minus1 + 1);

    if(RIdx > MAX_REF_PIC_SET)
      return false;

    // num negative pics computation
    for(int32_t j = pSPS->NumPositivePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        if(num_negative >= AL_MAX_REF)
          return false;

        pSPS->DeltaPocS0[RpsIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RpsIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    if(DeltaRPS < 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      if(num_negative >= AL_MAX_REF)
        return false;

      pSPS->DeltaPocS0[RpsIdx][num_negative] = DeltaRPS;
      pSPS->UsedByCurrPicS0[RpsIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(int32_t j = 0; j < pSPS->NumNegativePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc < 0 && ref_pic_set.use_delta_flag[j])
      {
        if(num_negative >= AL_MAX_REF)
          return false;

        pSPS->DeltaPocS0[RpsIdx][num_negative] = delta_poc;
        pSPS->UsedByCurrPicS0[RpsIdx][num_negative++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    pSPS->NumNegativePics[RpsIdx] = num_negative;

    // num positive pics computation
    for(int32_t j = pSPS->NumNegativePics[RIdx] - 1; j >= 0; --j)
    {
      int32_t delta_poc = pSPS->DeltaPocS0[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[j])
      {
        if(num_negative >= AL_MAX_REF)
          return false;

        pSPS->DeltaPocS1[RpsIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RpsIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[j];
      }
    }

    if(DeltaRPS > 0 && ref_pic_set.use_delta_flag[pSPS->NumDeltaPocs[RIdx]])
    {
      if(num_negative >= AL_MAX_REF)
        return false;

      pSPS->DeltaPocS1[RpsIdx][num_positive] = DeltaRPS;
      pSPS->UsedByCurrPicS1[RpsIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumDeltaPocs[RIdx]];
    }

    for(int32_t j = 0; j < pSPS->NumPositivePics[RIdx]; ++j)
    {
      int32_t delta_poc = pSPS->DeltaPocS1[RIdx][j] + DeltaRPS;

      if(delta_poc > 0 && ref_pic_set.use_delta_flag[pSPS->NumNegativePics[RIdx] + j])
      {
        if(num_negative >= AL_MAX_REF)
          return false;

        pSPS->DeltaPocS1[RpsIdx][num_positive] = delta_poc;
        pSPS->UsedByCurrPicS1[RpsIdx][num_positive++] = ref_pic_set.used_by_curr_pic_flag[pSPS->NumNegativePics[RIdx] + j];
      }
    }

    pSPS->NumPositivePics[RpsIdx] = num_positive;
  }
  else
  {
    pSPS->NumNegativePics[RpsIdx] = ref_pic_set.num_negative_pics;
    pSPS->NumPositivePics[RpsIdx] = ref_pic_set.num_positive_pics;

    pSPS->UsedByCurrPicS0[RpsIdx][0] = ref_pic_set.used_by_curr_pic_s0_flag[0];
    pSPS->UsedByCurrPicS1[RpsIdx][0] = ref_pic_set.used_by_curr_pic_s1_flag[0];

    pSPS->DeltaPocS0[RpsIdx][0] = -(ref_pic_set.delta_poc_s0_minus1[0] + 1);
    pSPS->DeltaPocS1[RpsIdx][0] = ref_pic_set.delta_poc_s1_minus1[0] + 1;

    for(int32_t j = 1; j < ref_pic_set.num_negative_pics; ++j)
    {
      pSPS->UsedByCurrPicS0[RpsIdx][j] = ref_pic_set.used_by_curr_pic_s0_flag[j];
      pSPS->DeltaPocS0[RpsIdx][j] = pSPS->DeltaPocS0[RpsIdx][j - 1] - (ref_pic_set.delta_poc_s0_minus1[j] + 1);
    }

    for(int32_t j = 1; j < ref_pic_set.num_positive_pics; ++j)
    {
      pSPS->UsedByCurrPicS1[RpsIdx][j] = ref_pic_set.used_by_curr_pic_s1_flag[j];
      pSPS->DeltaPocS1[RpsIdx][j] = pSPS->DeltaPocS1[RpsIdx][j - 1] + (ref_pic_set.delta_poc_s1_minus1[j] + 1);
    }
  }
  pSPS->NumDeltaPocs[RpsIdx] = pSPS->NumNegativePics[RpsIdx] + pSPS->NumPositivePics[RpsIdx];

  return true;
}

/*****************************************************************************/
static void hevc_profile_tier_level(AL_THevcProfilevel* pPrfLvl, int32_t iMaxSubLayersMinus1, AL_TRbspParser* pRP)
{
  pPrfLvl->general_profile_space = u(pRP, 2);
  pPrfLvl->general_tier_flag = u(pRP, 1);
  pPrfLvl->general_profile_idc = u(pRP, 5);

  for(int32_t i = 0; i < 32; ++i)
    pPrfLvl->general_profile_compatibility_flag[i] = u(pRP, 1);

  pPrfLvl->general_progressive_source_flag = u(pRP, 1);
  pPrfLvl->general_interlaced_source_flag = u(pRP, 1);
  pPrfLvl->general_non_packed_constraint_flag = u(pRP, 1);
  pPrfLvl->general_frame_only_constraint_flag = u(pRP, 1);

  pPrfLvl->general_max_12bit_constraint_flag = 0;
  pPrfLvl->general_max_10bit_constraint_flag = 0;
  pPrfLvl->general_max_8bit_constraint_flag = 0;
  pPrfLvl->general_max_422chroma_constraint_flag = 0;
  pPrfLvl->general_max_420chroma_constraint_flag = 0;
  pPrfLvl->general_max_monochrome_constraint_flag = 0;
  pPrfLvl->general_intra_constraint_flag = 0;
  pPrfLvl->general_one_picture_only_constraint_flag = 0;
  pPrfLvl->general_lower_bit_rate_constraint_flag = 0;
  pPrfLvl->general_max_14bit_constraint_flag = 0;
  pPrfLvl->general_inbld_flag = 0;

  if(pPrfLvl->general_profile_idc == 4 || pPrfLvl->general_profile_compatibility_flag[4] ||
     pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
     pPrfLvl->general_profile_idc == 6 || pPrfLvl->general_profile_compatibility_flag[6] ||
     pPrfLvl->general_profile_idc == 7 || pPrfLvl->general_profile_compatibility_flag[7] ||
     pPrfLvl->general_profile_idc == 8 || pPrfLvl->general_profile_compatibility_flag[8] ||
     pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
     pPrfLvl->general_profile_idc == 10 || pPrfLvl->general_profile_compatibility_flag[10] ||
     pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
  {
    pPrfLvl->general_max_12bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_10bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_8bit_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_422chroma_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_420chroma_constraint_flag = u(pRP, 1);
    pPrfLvl->general_max_monochrome_constraint_flag = u(pRP, 1);
    pPrfLvl->general_intra_constraint_flag = u(pRP, 1);
    pPrfLvl->general_one_picture_only_constraint_flag = u(pRP, 1);
    pPrfLvl->general_lower_bit_rate_constraint_flag = u(pRP, 1);

    if(pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
       pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
       pPrfLvl->general_profile_idc == 10 || pPrfLvl->general_profile_compatibility_flag[10] ||
       pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
    {
      pPrfLvl->general_max_14bit_constraint_flag = u(pRP, 1);
      skip(pRP, 33); // general_reserved_zero_33bits
    }
    else
      skip(pRP, 34); // general_reserved_zero_34bits
  }
  else if(pPrfLvl->general_profile_idc == 2 || pPrfLvl->general_profile_compatibility_flag[2])
  {
    skip(pRP, 7); // general_reserved_zero_7bits
    pPrfLvl->general_one_picture_only_constraint_flag = u(pRP, 1);
    skip(pRP, 35); // general_reserved_zero_35bits
  }
  else
    skip(pRP, 43); // general_reserved_zero_43bits

  if(pPrfLvl->general_profile_idc == 1 || pPrfLvl->general_profile_compatibility_flag[1] ||
     pPrfLvl->general_profile_idc == 2 || pPrfLvl->general_profile_compatibility_flag[2] ||
     pPrfLvl->general_profile_idc == 3 || pPrfLvl->general_profile_compatibility_flag[3] ||
     pPrfLvl->general_profile_idc == 4 || pPrfLvl->general_profile_compatibility_flag[4] ||
     pPrfLvl->general_profile_idc == 5 || pPrfLvl->general_profile_compatibility_flag[5] ||
     pPrfLvl->general_profile_idc == 9 || pPrfLvl->general_profile_compatibility_flag[9] ||
     pPrfLvl->general_profile_idc == 11 || pPrfLvl->general_profile_compatibility_flag[11])
  {
    pPrfLvl->general_inbld_flag = u(pRP, 1);
  }
  else
    skip(pRP, 1); // general_reserved_zero_bit

  pPrfLvl->general_level_idc = u(pRP, 8);

  for(int32_t i = 0; i < iMaxSubLayersMinus1; i++)
  {
    Rtos_Assert(iMaxSubLayersMinus1 <= AL_MAX_SUB_LAYER);
    pPrfLvl->sub_layer_profile_present_flag[i] = u(pRP, 1);
    pPrfLvl->sub_layer_level_present_flag[i] = u(pRP, 1);
  }

  if(iMaxSubLayersMinus1 > 0)
  {
    for(int32_t i = iMaxSubLayersMinus1; i <= AL_MAX_SUB_LAYER; i++)
      skip(pRP, 2); // reserved_zero_2_bits
  }

  for(int32_t i = 0; i < iMaxSubLayersMinus1; ++i)
  {
    if(pPrfLvl->sub_layer_profile_present_flag[i])
    {
      pPrfLvl->sub_layer_profile_space[i] = u(pRP, 2);
      pPrfLvl->sub_layer_tier_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_profile_idc[i] = u(pRP, 5);

      for(int32_t j = 0; j < 32; ++j)
        pPrfLvl->sub_layer_profile_compatibility_flag[i][j] = u(pRP, 1);

      pPrfLvl->sub_layer_progressive_source_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_interlaced_source_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_non_packed_constraint_flag[i] = u(pRP, 1);
      pPrfLvl->sub_layer_frame_only_constraint_flag[i] = u(pRP, 1);

      pPrfLvl->sub_layer_max_12bit_constraint_flag[i] = pPrfLvl->general_max_12bit_constraint_flag;
      pPrfLvl->sub_layer_max_10bit_constraint_flag[i] = pPrfLvl->general_max_10bit_constraint_flag;
      pPrfLvl->sub_layer_max_8bit_constraint_flag[i] = pPrfLvl->general_max_8bit_constraint_flag;
      pPrfLvl->sub_layer_max_422chroma_constraint_flag[i] = pPrfLvl->general_max_422chroma_constraint_flag;
      pPrfLvl->sub_layer_max_420chroma_constraint_flag[i] = pPrfLvl->general_max_420chroma_constraint_flag;
      pPrfLvl->sub_layer_max_monochrome_constraint_flag[i] = pPrfLvl->general_max_monochrome_constraint_flag;
      pPrfLvl->sub_layer_intra_constraint_flag[i] = pPrfLvl->general_intra_constraint_flag;
      pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = pPrfLvl->general_one_picture_only_constraint_flag;
      pPrfLvl->sub_layer_lower_bit_rate_constraint_flag[i] = pPrfLvl->general_lower_bit_rate_constraint_flag;
      pPrfLvl->sub_layer_max_14bit_constraint_flag[i] = pPrfLvl->general_max_14bit_constraint_flag;
      pPrfLvl->sub_layer_inbld_flag[i] = pPrfLvl->general_inbld_flag;

      if(pPrfLvl->sub_layer_profile_idc[i] == 4 || pPrfLvl->sub_layer_profile_compatibility_flag[i][4] ||
         pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
         pPrfLvl->sub_layer_profile_idc[i] == 6 || pPrfLvl->sub_layer_profile_compatibility_flag[i][6] ||
         pPrfLvl->sub_layer_profile_idc[i] == 7 || pPrfLvl->sub_layer_profile_compatibility_flag[i][7] ||
         pPrfLvl->sub_layer_profile_idc[i] == 8 || pPrfLvl->sub_layer_profile_compatibility_flag[i][8] ||
         pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
         pPrfLvl->sub_layer_profile_idc[i] == 10 || pPrfLvl->sub_layer_profile_compatibility_flag[i][10] ||
         pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
      {
        pPrfLvl->sub_layer_max_12bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_10bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_8bit_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_422chroma_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_420chroma_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_max_monochrome_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_intra_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = u(pRP, 1);
        pPrfLvl->sub_layer_lower_bit_rate_constraint_flag[i] = u(pRP, 1);

        if(pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
           pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
           pPrfLvl->sub_layer_profile_idc[i] == 10 || pPrfLvl->sub_layer_profile_compatibility_flag[i][10] ||
           pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
        {
          pPrfLvl->sub_layer_max_14bit_constraint_flag[i] = u(pRP, 1);
          skip(pRP, 33); // sub_layer_reserved_zero_33bits
        }
        else
          skip(pRP, 34); // sub_layer_reserved_zero_34bits
      }
      else if(pPrfLvl->sub_layer_profile_idc[i] == 2 || pPrfLvl->sub_layer_profile_compatibility_flag[i][2])
      {
        skip(pRP, 7); // sub_layer_reserved_zero_7bits
        pPrfLvl->sub_layer_one_picture_only_constraint_flag[i] = u(pRP, 1);
        skip(pRP, 35); // sub_layer_reserved_zero_35bits
      }
      else
        skip(pRP, 43); // sub_layer_reserved_zero_43bits

      if(pPrfLvl->sub_layer_profile_idc[i] == 1 || pPrfLvl->sub_layer_profile_compatibility_flag[i][1] ||
         pPrfLvl->sub_layer_profile_idc[i] == 2 || pPrfLvl->sub_layer_profile_compatibility_flag[i][2] ||
         pPrfLvl->sub_layer_profile_idc[i] == 3 || pPrfLvl->sub_layer_profile_compatibility_flag[i][3] ||
         pPrfLvl->sub_layer_profile_idc[i] == 4 || pPrfLvl->sub_layer_profile_compatibility_flag[i][4] ||
         pPrfLvl->sub_layer_profile_idc[i] == 5 || pPrfLvl->sub_layer_profile_compatibility_flag[i][5] ||
         pPrfLvl->sub_layer_profile_idc[i] == 9 || pPrfLvl->sub_layer_profile_compatibility_flag[i][9] ||
         pPrfLvl->sub_layer_profile_idc[i] == 11 || pPrfLvl->sub_layer_profile_compatibility_flag[i][11])
      {
        pPrfLvl->sub_layer_inbld_flag[i] = u(pRP, 1);
      }
      else
        skip(pRP, 1); // sub_layer_reserved_zero_bit
    }

    if(pPrfLvl->sub_layer_level_present_flag[i])
      pPrfLvl->sub_layer_level_idc[i] = u(pRP, 8);
  }
}

/*****************************************************************************/
static void initSps(AL_THevcSps* pSPS)
{
  Rtos_Memset(pSPS, 0, sizeof(AL_THevcSps));

  pSPS->chroma_format_idc = 1;

  pSPS->bConceal = true;
}

/*****************************************************************************/
static void initVui(AL_TVuiParam* pVuiParam, AL_THevcProfilevel* pProfileAndLevel)
{
  // The full SPS is already memset to 0 in InitSPS. Only nonzero values are overwritten here
  pVuiParam->video_format = 5;
  pVuiParam->colour_primaries = 2;
  pVuiParam->transfer_characteristics = 2;
  pVuiParam->frame_field_info_present_flag = (pProfileAndLevel->general_progressive_source_flag && pProfileAndLevel->general_interlaced_source_flag) ? 1 : 0;

  pVuiParam->max_bytes_per_pic_denom = 2;
  pVuiParam->max_bits_per_min_cu_denom = 1;
  pVuiParam->log2_max_mv_length_horizontal = 15;
  pVuiParam->log2_max_mv_length_vertical = 15;

  pVuiParam->hrd_param.du_cpb_removal_delay_increment_length_minus1 = 23;
  pVuiParam->hrd_param.initial_cpb_removal_delay_length_minus1 = 23;
  pVuiParam->hrd_param.dpb_output_delay_length_minus1 = 23;
}

/*****************************************************************************/
static void hevc_sub_hrd_parameters(AL_TSubHrdParam* pSubHrdParam, int32_t cpb_cnt, uint8_t sub_pic_hrd_params_present_flag, AL_TRbspParser* pRP)
{
  for(int32_t i = 0; i <= cpb_cnt; ++i)
  {
    pSubHrdParam->bit_rate_value_minus1[i] = ue(pRP);
    pSubHrdParam->cpb_size_value_minus1[i] = ue(pRP);

    if(sub_pic_hrd_params_present_flag)
    {
      pSubHrdParam->cpb_size_du_value_minus1[i] = ue(pRP);
      pSubHrdParam->bit_rate_du_value_minus1[i] = ue(pRP);
    }
    pSubHrdParam->cbr_flag[i] = u(pRP, 1);
  }
}

/*****************************************************************************/
static void hevc_hrd_parameters(AL_THrdParam* pHrdParam, bool bInfoFlag, int32_t iMaxSubLayersMinus1, AL_TRbspParser* pRP)
{
  if(bInfoFlag)
  {
    pHrdParam->nal_hrd_parameters_present_flag = u(pRP, 1);
    pHrdParam->vcl_hrd_parameters_present_flag = u(pRP, 1);

    if(pHrdParam->nal_hrd_parameters_present_flag || pHrdParam->vcl_hrd_parameters_present_flag)
    {
      pHrdParam->sub_pic_hrd_params_present_flag = u(pRP, 1);

      if(pHrdParam->sub_pic_hrd_params_present_flag)
      {
        pHrdParam->tick_divisor_minus2 = u(pRP, 8);
        pHrdParam->du_cpb_removal_delay_increment_length_minus1 = u(pRP, 5);
        pHrdParam->sub_pic_cpb_params_in_pic_timing_sei_flag = u(pRP, 1);
        pHrdParam->dpb_output_delay_du_length_minus1 = u(pRP, 5);
      }

      pHrdParam->bit_rate_scale = u(pRP, 4);
      pHrdParam->cpb_size_du_scale = u(pRP, 4);

      if(pHrdParam->sub_pic_hrd_params_present_flag)
        pHrdParam->cpb_size_scale = u(pRP, 4);
      pHrdParam->initial_cpb_removal_delay_length_minus1 = u(pRP, 5);
      pHrdParam->au_cpb_removal_delay_length_minus1 = u(pRP, 5);
      pHrdParam->dpb_output_delay_length_minus1 = u(pRP, 5);
    }
  }
  else
  {
    pHrdParam->nal_hrd_parameters_present_flag = 0;
    pHrdParam->vcl_hrd_parameters_present_flag = 0;
  }

  for(int32_t i = 0; i <= iMaxSubLayersMinus1; ++i)
  {
    /* initialization */
    pHrdParam->low_delay_hrd_flag[i] = 0;
    pHrdParam->cpb_cnt_minus1[i] = 0;

    pHrdParam->fixed_pic_rate_general_flag[i] = u(pRP, 1);

    if(!pHrdParam->fixed_pic_rate_general_flag[i])
      pHrdParam->fixed_pic_rate_within_cvs_flag[i] = u(pRP, 1);
    else
      pHrdParam->fixed_pic_rate_within_cvs_flag[i] = 1;

    if(pHrdParam->fixed_pic_rate_within_cvs_flag[i])
      pHrdParam->elemental_duration_in_tc_minus1[i] = ue(pRP);
    else
      pHrdParam->low_delay_hrd_flag[i] = u(pRP, 1);

    if(!pHrdParam->low_delay_hrd_flag[i])
      pHrdParam->cpb_cnt_minus1[i] = ue(pRP);

    /* Concealment: E.2.2 : cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive */
    pHrdParam->cpb_cnt_minus1[i] = UnsignedMin(pHrdParam->cpb_cnt_minus1[i], 31);

    if(pHrdParam->nal_hrd_parameters_present_flag)
      hevc_sub_hrd_parameters(&pHrdParam->nal_sub_hrd_param, pHrdParam->cpb_cnt_minus1[i], pHrdParam->sub_pic_hrd_params_present_flag, pRP);

    if(pHrdParam->vcl_hrd_parameters_present_flag)
      hevc_sub_hrd_parameters(&pHrdParam->vcl_sub_hrd_param, pHrdParam->cpb_cnt_minus1[i], pHrdParam->sub_pic_hrd_params_present_flag, pRP);
  }
}

/*****************************************************************************/
static void hevc_vui_parameters(AL_TVuiParam* pVuiParam, int32_t iMaxSubLayersMinus1, AL_TRbspParser* pRP)
{
  pVuiParam->aspect_ratio_info_present_flag = u(pRP, 1);

  if(pVuiParam->aspect_ratio_info_present_flag)
  {
    pVuiParam->aspect_ratio_idc = u(pRP, 8);

    if(pVuiParam->aspect_ratio_idc == 255)
    {
      pVuiParam->sar_width = u(pRP, 16);
      pVuiParam->sar_height = u(pRP, 16);
    }
  }

  pVuiParam->overscan_info_present_flag = u(pRP, 1);

  if(pVuiParam->overscan_info_present_flag)
    pVuiParam->overscan_appropriate_flag = u(pRP, 1);

  pVuiParam->video_signal_type_present_flag = u(pRP, 1);

  if(pVuiParam->video_signal_type_present_flag)
  {
    pVuiParam->video_format = u(pRP, 3);
    pVuiParam->video_full_range_flag = u(pRP, 1);
    pVuiParam->colour_description_present_flag = u(pRP, 1);

    if(pVuiParam->colour_description_present_flag)
    {
      pVuiParam->colour_primaries = u(pRP, 8);
      pVuiParam->transfer_characteristics = u(pRP, 8);
      pVuiParam->matrix_coefficients = u(pRP, 8);
    }
  }

  pVuiParam->chroma_loc_info_present_flag = u(pRP, 1);

  if(pVuiParam->chroma_loc_info_present_flag)
  {
    pVuiParam->chroma_sample_loc_type_top_field = ue(pRP);
    pVuiParam->chroma_sample_loc_type_bottom_field = ue(pRP);
  }
  pVuiParam->neutral_chroma_indication_flag = u(pRP, 1);
  pVuiParam->field_seq_flag = u(pRP, 1);
  pVuiParam->frame_field_info_present_flag = u(pRP, 1);

  pVuiParam->default_display_window_flag = u(pRP, 1);

  if(pVuiParam->default_display_window_flag)
  {
    pVuiParam->def_disp_win_left_offset = ue(pRP);
    pVuiParam->def_disp_win_right_offset = ue(pRP);
    pVuiParam->def_disp_win_top_offset = ue(pRP);
    pVuiParam->def_disp_win_bottom_offset = ue(pRP);
  }

  pVuiParam->vui_timing_info_present_flag = u(pRP, 1);

  if(pVuiParam->vui_timing_info_present_flag)
  {
    pVuiParam->vui_num_units_in_tick = u(pRP, 32);
    pVuiParam->vui_time_scale = u(pRP, 32);
    pVuiParam->vui_poc_proportional_to_timing_flag = u(pRP, 1);

    if(pVuiParam->vui_poc_proportional_to_timing_flag)
      pVuiParam->vui_num_ticks_poc_diff_one_minus1 = ue(pRP);

    pVuiParam->vui_hrd_parameters_present_flag = u(pRP, 1);

    if(pVuiParam->vui_hrd_parameters_present_flag)
      hevc_hrd_parameters(&pVuiParam->hrd_param, true, iMaxSubLayersMinus1, pRP);
  }

  pVuiParam->bitstream_restriction_flag = u(pRP, 1);

  if(pVuiParam->bitstream_restriction_flag)
  {
    pVuiParam->tiles_fixed_structure_flag = u(pRP, 1);
    pVuiParam->motion_vectors_over_pic_boundaries_flag = u(pRP, 1);
    pVuiParam->restricted_ref_pic_lists_flag = u(pRP, 1);
    pVuiParam->min_spatial_segmentation_idc = ue(pRP);
    pVuiParam->max_bytes_per_pic_denom = ue(pRP);
    pVuiParam->max_bits_per_min_cu_denom = ue(pRP);
    pVuiParam->log2_max_mv_length_horizontal = ue(pRP);
    pVuiParam->log2_max_mv_length_vertical = ue(pRP);
  }
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_TRbspParser* pRP, AL_THevcSps* pSPS)
{
  skipAllZerosAndTheNextByte(pRP);
  u(pRP, 16); // Skip NUT + temporal_id

  initSps(pSPS);

  pSPS->sps_video_parameter_set_id = u(pRP, 4);
  COMPLY_ID(pSPS->sps_video_parameter_set_id < AL_HEVC_MAX_VPS);

  int32_t max_sub_layers = Clip3(u(pRP, 3), 0, AL_MAX_SUB_LAYER - 1);
  pSPS->sps_max_sub_layers_minus1 = max_sub_layers;
  pSPS->sps_temporal_id_nesting_flag = u(pRP, 1);

  hevc_profile_tier_level(&pSPS->profile_and_level, pSPS->sps_max_sub_layers_minus1, pRP);

  if(pSPS->profile_and_level.general_level_idc == 0)
    pSPS->profile_and_level.general_level_idc = CONCEAL_LEVEL_IDC;

  pSPS->sps_seq_parameter_set_id = ue(pRP);
  COMPLY_ID(pSPS->sps_seq_parameter_set_id < AL_HEVC_MAX_SPS);

  // default VUI values
  initVui(&pSPS->vui_param, &pSPS->profile_and_level);

  pSPS->chroma_format_idc = ue(pRP);

  COMPLY(pSPS->chroma_format_idc < AL_CHROMA_MAX_ENUM);

  if(pSPS->chroma_format_idc == 3)
    pSPS->separate_colour_plane_flag = u(pRP, 1);

  if(pSPS->separate_colour_plane_flag)
    pSPS->ChromaArrayType = 0;
  else
    pSPS->ChromaArrayType = pSPS->chroma_format_idc;

  pSPS->pic_width_in_luma_samples = ue(pRP);
  pSPS->pic_height_in_luma_samples = ue(pRP);

  pSPS->conformance_window_flag = u(pRP, 1);

  if(pSPS->conformance_window_flag)
  {
    pSPS->conf_win_left_offset = ue(pRP);
    pSPS->conf_win_right_offset = ue(pRP);
    pSPS->conf_win_top_offset = ue(pRP);
    pSPS->conf_win_bottom_offset = ue(pRP);

    if(pSPS->conf_win_bottom_offset + pSPS->conf_win_top_offset >= pSPS->pic_height_in_luma_samples)
    {
      pSPS->conf_win_top_offset = 0;
      pSPS->conf_win_bottom_offset = 0;
    }

    if(pSPS->conf_win_left_offset + pSPS->conf_win_right_offset >= pSPS->pic_width_in_luma_samples)
    {
      pSPS->conf_win_left_offset = 0;
      pSPS->conf_win_right_offset = 0;
    }
  }

  pSPS->bit_depth_luma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH_MINUS_8);
  pSPS->bit_depth_chroma_minus8 = Clip3(ue(pRP), 0, MAX_BIT_DEPTH_MINUS_8);

  pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 = ue(pRP);

  COMPLY(pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 <= MAX_POC_LSB_MINUS_4);

  pSPS->sps_sub_layer_ordering_info_present_flag = u(pRP, 1);
  int32_t layer_offset = pSPS->sps_sub_layer_ordering_info_present_flag ? 0 : max_sub_layers;

  for(int32_t i = layer_offset; i <= max_sub_layers; ++i)
  {
    pSPS->sps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    pSPS->sps_max_num_reorder_pics[i] = ue(pRP);
    pSPS->sps_max_latency_increase_plus1[i] = ue(pRP);
  }

  if(!pSPS->sps_sub_layer_ordering_info_present_flag)
  {
    for(int32_t i = 0; i < layer_offset; ++i)
    {
      pSPS->sps_max_dec_pic_buffering_minus1[i] = pSPS->sps_max_dec_pic_buffering_minus1[layer_offset];
      pSPS->sps_max_num_reorder_pics[i] = pSPS->sps_max_num_reorder_pics[layer_offset];
      pSPS->sps_max_latency_increase_plus1[i] = pSPS->sps_max_latency_increase_plus1[layer_offset];
    }
  }

  pSPS->log2_min_luma_coding_block_size_minus3 = ue(pRP);
  pSPS->Log2MinCbSize = pSPS->log2_min_luma_coding_block_size_minus3 + 3;

  COMPLY(pSPS->Log2MinCbSize <= 6);

  pSPS->log2_diff_max_min_luma_coding_block_size = ue(pRP);
  pSPS->Log2CtbSize = pSPS->Log2MinCbSize + pSPS->log2_diff_max_min_luma_coding_block_size;

  COMPLY(pSPS->Log2CtbSize <= 6);
  COMPLY(pSPS->Log2CtbSize >= 4);

  pSPS->log2_min_transform_block_size_minus2 = ue(pRP);
  int32_t Log2MinTransfoSize = pSPS->log2_min_transform_block_size_minus2 + 2;
  pSPS->log2_diff_max_min_transform_block_size = ue(pRP);

  COMPLY(pSPS->log2_min_transform_block_size_minus2 <= pSPS->log2_min_luma_coding_block_size_minus3);
  COMPLY(pSPS->log2_diff_max_min_transform_block_size <= Min(pSPS->Log2CtbSize, 5) - Log2MinTransfoSize);

  pSPS->max_transform_hierarchy_depth_inter = ue(pRP);
  pSPS->max_transform_hierarchy_depth_intra = ue(pRP);

  COMPLY(pSPS->max_transform_hierarchy_depth_inter <= (pSPS->Log2CtbSize - Log2MinTransfoSize));
  COMPLY(pSPS->max_transform_hierarchy_depth_intra <= (pSPS->Log2CtbSize - Log2MinTransfoSize));

  pSPS->scaling_list_enabled_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(pSPS->scaling_list_enabled_flag)
  {
    pSPS->sps_scaling_list_data_present_flag = u(pRP, 1);

    if(pSPS->sps_scaling_list_data_present_flag)
      hevc_scaling_list_data(&pSPS->scaling_list_param, pRP);
    else
      for(int32_t i = 0; i < 20; ++i)
        pSPS->scaling_list_param.UseDefaultScalingMatrixFlag[i] = 1;
  }

  pSPS->amp_enabled_flag = u(pRP, 1);
  pSPS->sample_adaptive_offset_enabled_flag = u(pRP, 1);

  pSPS->pcm_enabled_flag = u(pRP, 1);

  if(pSPS->pcm_enabled_flag)
  {
    pSPS->pcm_sample_bit_depth_luma_minus1 = u(pRP, 4);
    pSPS->pcm_sample_bit_depth_chroma_minus1 = u(pRP, 4);

    COMPLY(pSPS->pcm_sample_bit_depth_luma_minus1 <= pSPS->bit_depth_luma_minus8 + 7);
    COMPLY(pSPS->pcm_sample_bit_depth_chroma_minus1 <= pSPS->bit_depth_chroma_minus8 + 7);

    pSPS->log2_min_pcm_luma_coding_block_size_minus3 = Clip3(ue(pRP), Min(pSPS->log2_min_luma_coding_block_size_minus3, 2), Min(pSPS->Log2CtbSize - 3, 2));

    COMPLY(pSPS->log2_min_pcm_luma_coding_block_size_minus3 >= Min(pSPS->log2_min_luma_coding_block_size_minus3, 2));
    COMPLY(pSPS->log2_min_pcm_luma_coding_block_size_minus3 <= Min(pSPS->Log2CtbSize - 3, 2));

    pSPS->log2_diff_max_min_pcm_luma_coding_block_size = Clip3(ue(pRP), 0, Min(pSPS->Log2CtbSize - 3, 2) - pSPS->log2_min_pcm_luma_coding_block_size_minus3);

    COMPLY(pSPS->log2_diff_max_min_pcm_luma_coding_block_size <= (Min(pSPS->Log2CtbSize - 3, 2) - pSPS->log2_min_pcm_luma_coding_block_size_minus3));

    pSPS->pcm_loop_filter_disabled_flag = u(pRP, 1);
  }

  pSPS->num_short_term_ref_pic_sets = ue(pRP);

  COMPLY(pSPS->num_short_term_ref_pic_sets <= MAX_REF_PIC_SET);

  for(int32_t i = 0; i < pSPS->num_short_term_ref_pic_sets; ++i)
  {
    // check if NAL isn't empty
    COMPLY(more_rbsp_data(pRP));
    COMPLY(AL_HEVC_short_term_ref_pic_set(pSPS, i, pRP));

    pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1] =
      Max(pSPS->sps_max_dec_pic_buffering_minus1[pSPS->sps_max_sub_layers_minus1], pSPS->NumDeltaPocs[i]);
  }

  pSPS->long_term_ref_pics_present_flag = u(pRP, 1);

  if(pSPS->long_term_ref_pics_present_flag)
  {
    uint8_t syntax_size = pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 + 4;
    pSPS->num_long_term_ref_pics_sps = ue(pRP);

    COMPLY(pSPS->num_long_term_ref_pics_sps <= MAX_LONG_TERM_PIC);

    for(int32_t i = 0; i < pSPS->num_long_term_ref_pics_sps; ++i)
    {
      pSPS->lt_ref_pic_poc_lsb_sps[i] = u(pRP, syntax_size);
      pSPS->used_by_curr_pic_lt_sps_flag[i] = u(pRP, 1);
    }
  }
  pSPS->sps_temporal_mvp_enabled_flag = u(pRP, 1);
  pSPS->strong_intra_smoothing_enabled_flag = u(pRP, 1);

  pSPS->vui_parameters_present_flag = u(pRP, 1);

  // check if NAL isn't empty
  COMPLY(more_rbsp_data(pRP));

  if(pSPS->vui_parameters_present_flag)
    hevc_vui_parameters(&pSPS->vui_param, pSPS->sps_max_sub_layers_minus1, pRP);

  pSPS->sps_extension_present_flag = u(pRP, 1);

  if(pSPS->sps_extension_present_flag)
  {
    pSPS->sps_range_extension_flag = u(pRP, 1);
    pSPS->sps_extension_7bits = u(pRP, 7);
  }

  if(pSPS->sps_range_extension_flag)
  {
    pSPS->transform_skip_rotation_enabled_flag = u(pRP, 1);
    pSPS->transform_skip_context_enabled_flag = u(pRP, 1);
    pSPS->implicit_rdpcm_enabled_flag = u(pRP, 1);
    pSPS->explicit_rdpcm_enabled_flag = u(pRP, 1);
    pSPS->extended_precision_processing_flag = u(pRP, 1);
    pSPS->intra_smoothing_disabled_flag = u(pRP, 1);
    pSPS->high_precision_offsets_enabled_flag = u(pRP, 1);
    pSPS->persistent_rice_adaptation_enabled_flag = u(pRP, 1);
    pSPS->cabac_bypass_alignment_enabled_flag = u(pRP, 1);
  }

  if(pSPS->sps_extension_7bits) // sps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // sps_extension_data_flag
  }

  // Compute variables
  pSPS->PicWidthInCtbs = (pSPS->pic_width_in_luma_samples + ((1 << pSPS->Log2CtbSize) - 1)) >> pSPS->Log2CtbSize;
  pSPS->PicHeightInCtbs = (pSPS->pic_height_in_luma_samples + ((1 << pSPS->Log2CtbSize) - 1)) >> pSPS->Log2CtbSize;

  COMPLY_WITH_LOG(pSPS->PicWidthInCtbs >= 2, "SPS width less than 2 CTBs is not supported\n");
  COMPLY_WITH_LOG(pSPS->PicHeightInCtbs >= 2, "SPS height less than 2 CTBs is not supported\n");

  pSPS->PicWidthInMinCbs = pSPS->pic_width_in_luma_samples >> pSPS->Log2MinCbSize;
  pSPS->PicHeightInMinCbs = pSPS->pic_height_in_luma_samples >> pSPS->Log2MinCbSize;

  pSPS->SpsMaxLatency = pSPS->sps_max_latency_increase_plus1[max_sub_layers] ? pSPS->sps_max_num_reorder_pics[max_sub_layers] + pSPS->sps_max_latency_increase_plus1[max_sub_layers] - 1 : UINT32_MAX;

  pSPS->MaxPicOrderCntLsb = 1 << (pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 + 4);

  pSPS->WpOffsetBdShiftY = pSPS->high_precision_offsets_enabled_flag ? 0 : pSPS->bit_depth_luma_minus8;
  pSPS->WpOffsetBdShiftC = pSPS->high_precision_offsets_enabled_flag ? 0 : pSPS->bit_depth_chroma_minus8;
  pSPS->WpOffsetHalfRangeY = 1 << (pSPS->high_precision_offsets_enabled_flag ? pSPS->bit_depth_luma_minus8 + 7 : 7);
  pSPS->WpOffsetHalfRangeC = 1 << (pSPS->high_precision_offsets_enabled_flag ? pSPS->bit_depth_chroma_minus8 + 7 : 7);

  COMPLY(rbsp_trailing_bits(pRP));

  pSPS->bConceal = false;

  return AL_OK;
}

/*****************************************************************************/
bool AL_HEVC_short_term_ref_pic_set(AL_THevcSps* pSPS, uint8_t RpsIdx, AL_TRbspParser* pRP)
{
  uint8_t RIdx;
  AL_TRefPicSet* pRefPicSet = &pSPS->short_term_ref_pic_set[RpsIdx];

  // default values
  pRefPicSet->delta_idx_minus1 = 0;
  pRefPicSet->inter_ref_pic_set_prediction_flag = 0;

  if(RpsIdx)
    pRefPicSet->inter_ref_pic_set_prediction_flag = u(pRP, 1);

  if(pRefPicSet->inter_ref_pic_set_prediction_flag)
  {
    if(RpsIdx == pSPS->num_short_term_ref_pic_sets)
      pRefPicSet->delta_idx_minus1 = ue(pRP);
    pRefPicSet->delta_rps_sign = u(pRP, 1);
    pRefPicSet->abs_delta_rps_minus1 = ue(pRP);

    RIdx = RpsIdx - (pRefPicSet->delta_idx_minus1 + 1);

    if(RIdx > MAX_REF_PIC_SET)
      return false;

    for(uint8_t j = 0; j <= pSPS->NumDeltaPocs[RIdx]; ++j)
    {
      pRefPicSet->use_delta_flag[j] = 1;
      pRefPicSet->used_by_curr_pic_flag[j] = u(pRP, 1);

      if(!pRefPicSet->used_by_curr_pic_flag[j])
        pRefPicSet->use_delta_flag[j] = u(pRP, 1);
    }
  }
  else
  {
    pRefPicSet->num_negative_pics = ue(pRP);

    if(pRefPicSet->num_negative_pics > AL_MAX_REF)
      return false;

    pRefPicSet->num_positive_pics = ue(pRP);

    if(pRefPicSet->num_negative_pics > AL_MAX_REF)
      return false;

    for(uint8_t j = 0; j < pRefPicSet->num_negative_pics; ++j)
    {
      pRefPicSet->delta_poc_s0_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s0_flag[j] = u(pRP, 1);
    }

    for(uint8_t j = 0; j < pRefPicSet->num_positive_pics; ++j)
    {
      pRefPicSet->delta_poc_s1_minus1[j] = ue(pRP);
      pRefPicSet->used_by_curr_pic_s1_flag[j] = u(pRP, 1);
    }
  }
  return AL_HEVC_sComputeRefPicSetVariables(pSPS, RpsIdx);
}

/*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseVPS(AL_TAup* pIAup, AL_TRbspParser* pRP)
{
  AL_THevcVps* pVPS;

  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  int32_t vps_id = u(pRP, 4);

  if(vps_id >= AL_HEVC_MAX_VPS)
    return AL_UNSUPPORTED;

  pVPS = &pIAup->hevcAup.pVPS[vps_id];
  pVPS->vps_video_parameter_set_id = vps_id;

  pVPS->vps_base_layer_internal_flag = u(pRP, 1);
  pVPS->vps_base_layer_available_flag = u(pRP, 1);
  pVPS->vps_max_layers_minus1 = u(pRP, 6);

  pVPS->vps_max_sub_layers_minus1 = u(pRP, 3);
  pVPS->vps_temporal_id_nesting_flag = u(pRP, 1);
  skip(pRP, 16); // vps_reserved_0xffff_16bits

  hevc_profile_tier_level(&pVPS->profile_and_level[0], pVPS->vps_max_sub_layers_minus1, pRP);

  if(pVPS->profile_and_level[0].general_level_idc == 0)
    pVPS->profile_and_level[0].general_level_idc = CONCEAL_LEVEL_IDC;

  pVPS->vps_sub_layer_ordering_info_present_flag = u(pRP, 1);

  int32_t layer_offset = pVPS->vps_sub_layer_ordering_info_present_flag ? 0 : pVPS->vps_max_sub_layers_minus1;

  for(int32_t i = layer_offset; i <= pVPS->vps_max_sub_layers_minus1; ++i)
  {
    pVPS->vps_max_dec_pic_buffering_minus1[i] = ue(pRP);
    pVPS->vps_max_num_reorder_pics[i] = ue(pRP);
    pVPS->vps_max_latency_increase_plus1[i] = ue(pRP);
  }

  pVPS->vps_max_layer_id = u(pRP, 6);
  pVPS->vps_num_layer_sets_minus1 = ue(pRP);

  for(int32_t i = 1; i <= pVPS->vps_num_layer_sets_minus1; ++i)
  {
    uint16_t uOffset = Min(i, 1);

    for(int32_t j = 0; j <= pVPS->vps_max_layer_id; j++)
      pVPS->layer_id_included_flag[uOffset][j] = u(pRP, 1);
  }

  pVPS->vps_timing_info_present_flag = u(pRP, 1);

  if(pVPS->vps_timing_info_present_flag)
  {
    pVPS->vps_num_units_in_tick = u(pRP, 32);
    pVPS->vps_time_scale = u(pRP, 32);
    pVPS->vps_poc_proportional_to_timing_flag = u(pRP, 1);

    if(pVPS->vps_poc_proportional_to_timing_flag)
      pVPS->vps_num_ticks_poc_diff_one_minus1 = ue(pRP);

    pVPS->vps_num_hrd_parameters = ue(pRP);

    for(int32_t i = 0; i < pVPS->vps_num_hrd_parameters; ++i)
    {
      uint16_t uOffset = Min(i, 1);
      pVPS->hrd_layer_set_idx[uOffset] = ue(pRP);

      if(uOffset)
        pVPS->cprms_present_flag[uOffset] = u(pRP, 1);
      else
        pVPS->cprms_present_flag[uOffset] = 1;
      hevc_hrd_parameters(&pVPS->hrd_parameter[uOffset], pVPS->cprms_present_flag[uOffset], pVPS->vps_max_sub_layers_minus1, pRP);
    }
  }

  if(u(pRP, 1)) // vps_extension_flag
  {
    while(more_rbsp_data(pRP))
      skip(pRP, 1); // vps_extension_data_flag
  }
  rbsp_trailing_bits(pRP);

  return AL_OK;
}

/*****************************************************************************/
static AL_PARSE_RESULT SeiActiveParameterSets(AL_TRbspParser* pRP, AL_THevcAup* aup, uint8_t* pSpsId)
{
  uint8_t active_video_parameter_set_id = u(pRP, 4);
  /*self_Contained_cvs_flag =*/ u(pRP, 1);
  /*no_parameter_set_update_flag =*/ u(pRP, 1);
  uint8_t num_sps_ids_minus1 = ue(pRP);
  COMPLY(num_sps_ids_minus1 < AL_HEVC_MAX_SPS);

  uint8_t active_seq_parameter_set_id[AL_HEVC_MAX_SPS];

  for(int32_t i = 0; i <= num_sps_ids_minus1; ++i)
  {
    active_seq_parameter_set_id[i] = ue(pRP);
    COMPLY(active_seq_parameter_set_id[i] < AL_HEVC_MAX_SPS);
  }

  AL_THevcVps const* pVPS = &aup->pVPS[active_video_parameter_set_id];
  uint8_t MaxLayersMinus1 = Min(62, pVPS->vps_max_layers_minus1);

  for(int32_t i = pVPS->vps_base_layer_internal_flag; i <= MaxLayersMinus1; ++i)
    /*layer_sps_idx[i] =*/ ue(pRP);

  COMPLY(aup->pSPS[active_seq_parameter_set_id[0]].bConceal == false);

  *pSpsId = active_seq_parameter_set_id[0];

  return AL_OK;
}

/*****************************************************************************/
static bool SeiPicTiming(AL_TRbspParser* pRP, AL_THevcSps* pSPS, AL_THevcPicTiming* pPicTiming)
{
  if(pSPS == NULL || pSPS->bConceal)
    return false;

  Rtos_Memset(pPicTiming, 0, sizeof(*pPicTiming));

  if(pSPS->vui_param.frame_field_info_present_flag)
  {
    pPicTiming->pic_struct = u(pRP, 4);
    pPicTiming->source_scan_type = u(pRP, 2);
    pPicTiming->duplicate_flag = u(pRP, 1);
    pSPS->sei_source_scan_type = pPicTiming->source_scan_type;
  }

  bool CpbDpbDelaysPresentFlag = pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag
                                 || pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag;

  if(CpbDpbDelaysPresentFlag)
  {
    uint8_t syntax_size = pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 + 1;
    pPicTiming->au_cpb_removal_delay_minus1 = u(pRP, syntax_size);

    syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 + 1;
    pPicTiming->pic_dpb_output_delay = u(pRP, syntax_size);

    if(pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag)
    {
      syntax_size = pSPS->vui_param.hrd_param.dpb_output_delay_du_length_minus1 + 1;
      pPicTiming->pic_dpb_output_du_delay = u(pRP, syntax_size);

      if(pSPS->vui_param.hrd_param.sub_pic_cpb_params_in_pic_timing_sei_flag)
      {
        pPicTiming->num_decoding_units_minus1 = ue(pRP);
        pPicTiming->du_common_cpb_removal_delay_flag = u(pRP, 1);

        if(pPicTiming->du_common_cpb_removal_delay_flag)
        {
          syntax_size = pSPS->vui_param.hrd_param.du_cpb_removal_delay_increment_length_minus1 + 1;
          pPicTiming->du_common_cpb_removal_delay_increment_minus1 = u(pRP, syntax_size);
        }

        for(uint32_t i = 0; i <= pPicTiming->num_decoding_units_minus1; ++i)
        {
          /*pPicTiming->num_nalus_in_du_minus1[i] = */
          ue(pRP);

          if(!pPicTiming->du_common_cpb_removal_delay_flag && i < pPicTiming->num_decoding_units_minus1)
            /*pPicTiming->du_cpb_removal_delay_increment_minus1[i] = */ u(pRP, syntax_size);
        }
      }
    }
  }
  return true;
}

/*****************************************************************************/
static bool ParseSeiPayload(SeiParserParam* p, AL_TRbspParser* pRP, AL_ESeiPayloadType ePayloadType, int32_t iPayloadSize, bool* bCanSendToUser, bool* bParsed)
{
  bool bParsingOk = true;
  AL_THevcAup* aup = &p->pIAup->hevcAup;
  *bCanSendToUser = true;
  *bParsed = true;
  switch(ePayloadType)
  {
  case SEI_PTYPE_PIC_TIMING: // picture_timing parsing
  {
    if(aup->pActiveSPS)
    {
      AL_THevcPicTiming tPictureTiming;
      bParsingOk = SeiPicTiming(pRP, aup->pActiveSPS, &tPictureTiming);

      if(bParsingOk)
        aup->ePicStruct = tPictureTiming.pic_struct;
    }
    else
      skip(pRP, iPayloadSize << 3);
    break;
  }
  case SEI_PTYPE_ACTIVE_PARAMETER_SETS:
  {
    uint8_t uSpsId;
    AL_PARSE_RESULT eResult = SeiActiveParameterSets(pRP, aup, &uSpsId);
    bParsingOk = eResult == AL_OK;

    if(bParsingOk)
      aup->pActiveSPS = &aup->pSPS[uSpsId];
    break;
  }
  default:
  {
    *bParsed = false;
    *bCanSendToUser = false;
    break;
  }
  }

  return bParsingOk;
}

/*****************************************************************************/
bool AL_HEVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta)
{
  skipAllZerosAndTheNextByte(pRP);

  u(pRP, 16); // Skip NUT + temporal_id

  SeiParserParam tUserParam = { pIAup, bIsPrefix, cb, pMeta };
  SeiParserCB tSeiParserCb = { ParseSeiPayload, &tUserParam };

  do
  {
    if(!ParseSeiHeader(pRP, &tSeiParserCb))
      return false;
  }
  while(more_rbsp_data(pRP));

  rbsp_trailing_bits(pRP);

  return true;
}

/*****************************************************************************/
void AL_HEVC_GetCropInfo(AL_THevcSps const* pSPS, AL_TCropInfo* pCropInfo)
{
  if(pSPS->conformance_window_flag)
  {
    pCropInfo->bCropping = true;

    if(pSPS->chroma_format_idc == 1 || pSPS->chroma_format_idc == 2)
    {
      pCropInfo->uCropOffsetLeft = 2 * pSPS->conf_win_left_offset;
      pCropInfo->uCropOffsetRight = 2 * pSPS->conf_win_right_offset;
    }
    else
    {
      pCropInfo->uCropOffsetLeft = pSPS->conf_win_left_offset;
      pCropInfo->uCropOffsetRight = pSPS->conf_win_right_offset;
    }

    if(pSPS->chroma_format_idc == 1)
    {
      pCropInfo->uCropOffsetTop = 2 * pSPS->conf_win_top_offset;
      pCropInfo->uCropOffsetBottom = 2 * pSPS->conf_win_bottom_offset;
    }
    else
    {
      pCropInfo->uCropOffsetTop = pSPS->conf_win_top_offset;
      pCropInfo->uCropOffsetBottom = pSPS->conf_win_bottom_offset;
    }
  }
  else
  {
    ResetCropInfo(pCropInfo);
  }
}
