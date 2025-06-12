// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "IP_Stream.h"
#include "lib_bitstream/IRbspWriter.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SEI.h"
#include "lib_common_enc/AUD.h"
#include "lib_common/HDR.h"

typedef struct AL_TNuts
{
  AL_TNalHeader (* GetNalHeader)(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId);
  int32_t spsNut;
  int32_t ppsNut;
  int32_t vpsNut;
  int32_t audNut;
  int32_t fdNut;
  int32_t seiPrefixNut;
  int32_t seiSuffixNut;
  int32_t phNut;
  int32_t apsNut;
}AL_TNuts;

typedef struct
{
  int32_t initialCpbRemovalDelay;
  int32_t cpbRemovalDelay;
  AL_THDRSEIs* pHDRSEIs;
}AL_TSeiData;

typedef struct
{
  AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned;
  AL_TVps* vps;
  AL_TSps* sps;
  AL_TPps* pps;
  AL_TAud* aud;

  bool bMustWriteAud;
  AL_EFillerCtrlMode fillerCtrlMode;
  bool bMustWritePPS;
  bool bMustWriteDynHDR;
  AL_ESeiFlag seiFlags;
  AL_TSeiData seiData;
}AL_TNalsData;

void GenerateSections(IRbspWriter* writer, AL_TNuts Nuts, AL_TNalsData const* pNalsData, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int32_t iLayerID, int32_t iNumSlices, bool bSubframeLatency, bool bForceSEIRecoveryPointOnIDR);
int32_t AL_WriteSeiSection(AL_ECodec eCodec, AL_TNuts nuts, AL_TBuffer* pStream, bool isPrefix, int32_t iPayloadType, uint8_t* pPayload, int32_t iPayloadSize, int32_t iTempId, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);
bool AL_CreateNuts(AL_TNuts* nuts, AL_EProfile eProfile);
