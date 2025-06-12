// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "common_syntax_elements.h"

/****************************************************************************/
#define AL_AVC_MAX_SPS 32

// define max sps value respect to AVC semantics
static int32_t const MAX_FRAME_NUM = 12;
static int32_t const MAX_POC_TYPE = 2;

static int32_t const AL_AVC_MAX_WP_IDC = 2;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.1.
*****************************************************************************/
typedef struct AL_TAvcSps
{
  int32_t profile_idc;
  uint8_t constraint_set0_flag;
  uint8_t constraint_set1_flag;
  uint8_t constraint_set2_flag;
  uint8_t constraint_set3_flag;
  uint8_t constraint_set4_flag;
  uint8_t constraint_set5_flag;
  uint8_t reserved_zero_2bits;
  int32_t level_idc;
  uint8_t seq_parameter_set_id;

  uint8_t chroma_format_idc;
  uint8_t separate_colour_plane_flag;

  uint8_t bit_depth_luma_minus8;
  uint8_t bit_depth_chroma_minus8;

  uint8_t qpprime_y_zero_transform_bypass_flag;

  uint8_t seq_scaling_matrix_present_flag;
  uint8_t seq_scaling_list_present_flag[12]; // we can define eight matrices: Sl_4x4_Intra_Y, Sl_4x4_Intra_Cb, Sl_4x4_Intra_Cr, Sl_4x4_Inter_Y, Sl_4x4_Inter_Cb, Sl_4x4_Inter_Cr, Sl_8x8_Intra_Y, Sl_8x8_Inter_Y.
  uint8_t ScalingList4x4[6][16]; // use in decoding
  uint8_t ScalingList8x8[6][64];
  AL_TSCLParam scaling_list_param;  // use in encoding

  uint8_t log2_max_frame_num_minus4;
  uint8_t pic_order_cnt_type;
  uint8_t log2_max_pic_order_cnt_lsb_minus4;
  uint8_t delta_pic_order_always_zero_flag;
  int32_t offset_for_non_ref_pic;
  int32_t offset_for_top_to_bottom_field;
  uint8_t num_ref_frames_in_pic_order_cnt_cycle;
  int32_t offset_for_ref_frame[256];
  uint32_t max_num_ref_frames;
  uint8_t gaps_in_frame_num_value_allowed_flag;

  uint16_t pic_width_in_mbs_minus1;
  uint16_t pic_height_in_map_units_minus1;
  uint8_t frame_mbs_only_flag;
  uint8_t field_pic_flag;
  uint8_t bottom_field_flag;
  uint8_t mb_adaptive_frame_field_flag;
  uint8_t direct_8x8_inference_flag;
  uint8_t frame_cropping_flag;
  int32_t frame_crop_left_offset;
  int32_t frame_crop_right_offset;
  int32_t frame_crop_top_offset;
  int32_t frame_crop_bottom_offset;

  uint8_t vui_parameters_present_flag;
  AL_TVuiParam vui_param;

  int32_t iCurrInitialCpbRemovalDelay; // Used in the current picture buffering_period
  uint8_t UseDefaultScalingMatrix4x4Flag[6];
  uint8_t UseDefaultScalingMatrix8x8Flag[6];
  // TSpsExtMVC mvc_ext;

  // concealment flag
  bool bConceal;
}AL_TAvcSps;

/*****************************************************************************/
#define AL_AVC_MAX_PPS 256
#define AL_AVC_MAX_REFERENCE_PICTURE_REORDER 17

static int32_t const AL_AVC_MAX_REF_IDX = 15;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.2.2.
*****************************************************************************/
typedef struct AL_TAvcPps
{
  uint8_t pic_parameter_set_id;
  uint8_t seq_parameter_set_id;
  uint8_t entropy_coding_mode_flag;
  uint8_t bottom_field_pic_order_in_frame_present_flag;

  int32_t num_slice_groups_minus1;

  uint8_t num_ref_idx_l0_active_minus1;
  uint8_t num_ref_idx_l1_active_minus1;

  uint8_t weighted_pred_flag;
  uint8_t weighted_bipred_idc;

  int8_t pic_init_qp_minus26;
  int8_t pic_init_qs_minus26;
  int8_t chroma_qp_index_offset;
  int8_t second_chroma_qp_index_offset;

  uint8_t deblocking_filter_control_present_flag;
  uint8_t constrained_intra_pred_flag;
  uint8_t redundant_pic_cnt_present_flag;

  uint8_t transform_8x8_mode_flag;

  uint8_t pic_scaling_matrix_present_flag;
  uint8_t pic_scaling_list_present_flag[12];
  uint8_t ScalingList4x4[6][16];
  uint8_t ScalingList8x8[6][64];
  uint8_t UseDefaultScalingMatrix4x4Flag[6];
  uint8_t UseDefaultScalingMatrix8x8Flag[6];

  AL_TAvcSps* pSPS;

  // concealment flag
  bool bConceal;
}AL_TAvcPps;

/*****************************************************************************
   \brief Mimics structure described in spec sec. 7.3.3
*****************************************************************************/
typedef struct AL_TAvcSliceHdr
{
  uint16_t num_line_in_slice;
  int32_t first_mb_in_slice;
  uint8_t slice_type; // 0 = P, 1 = B, 2 = I.
  uint8_t pic_parameter_set_id;
  uint8_t field_pic_flag;
  int32_t frame_num;
  int32_t redundant_pic_cnt;
  int32_t idr_pic_id;
  int32_t pic_order_cnt_lsb;
  int32_t delta_pic_order_cnt_bottom;
  int32_t delta_pic_order_cnt[2];
  uint8_t bottom_field_flag;
  uint8_t nal_ref_idc;
  uint8_t direct_spatial_mv_pred_flag;
  uint8_t num_ref_idx_active_override_flag;
  int32_t num_ref_idx_l0_active_minus1; // This member should always contain the current num_ref_idx_l0_active_minus1 applicable for this slice, even when num_ref_idx_active_override_flag == 1.
  int32_t num_ref_idx_l1_active_minus1; // This member should always contain the current num_ref_idx_l1_active_minus1 applicable for this slice, even when num_ref_idx_active_override_flag == 1.

  // reference picture list reordering syntax elements
  uint8_t reordering_of_pic_nums_idc_l0[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  uint8_t reordering_of_pic_nums_idc_l1[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  int32_t abs_diff_pic_num_minus1_l0[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  int32_t abs_diff_pic_num_minus1_l1[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  int32_t long_term_pic_num_l0[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  int32_t long_term_pic_num_l1[AL_AVC_MAX_REFERENCE_PICTURE_REORDER];
  uint8_t ref_pic_list_reordering_flag_l0;
  uint8_t ref_pic_list_reordering_flag_l1;

  // prediction weight table syntax elements
  AL_TWPTable pred_weight_table;

  // reference picture marking syntax elements
  uint8_t memory_management_control_operation[35];
  int32_t difference_of_pic_nums_minus1[32];
  int32_t long_term_pic_num[32];
  int32_t long_term_frame_idx[32];
  int32_t max_long_term_frame_idx_plus1[32];

  uint8_t no_output_of_prior_pics_flag;
  uint8_t long_term_reference_flag;
  uint8_t adaptive_ref_pic_marking_mode_flag;
  uint8_t cabac_init_idc;
  int32_t slice_qp_delta;
  uint8_t disable_deblocking_filter_idc;
  uint8_t nal_unit_type;
  int8_t slice_alpha_c0_offset_div2;
  int8_t slice_beta_offset_div2;
  int32_t slice_header_length;

  const AL_TAvcPps* pPPS;
  AL_TAvcSps* pSPS;
}AL_TAvcSliceHdr;

typedef struct AL_TAvcHdrSvcExt // nal_unit_header_svc_extensiont
{
  uint8_t idr_flag;
  uint8_t priority_id;
  uint8_t no_inter_layer_pred_flag;
  uint8_t dependency_id;
  uint8_t quality_id;
  uint8_t temporal_id;
  uint8_t use_ref_base_pic_flag;
  uint8_t discardable_flag;
  uint8_t output_flag;
}AL_TAvcHdrSvcExt;
