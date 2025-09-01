// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/PicFormat.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufConst.h"
#include "lib_common_dec/DecPicParam.h"
#include "I_PictMngr.h"

/*****************************************************************************/
#define PhysAddr AL_PADDR *

/*****************************************************************************/
void AL_CommonPictMngr_ExtractRefBuffersAddresses(AL_ECodec eCodec, AL_TPicFormat const* pPicFormat, AL_TPosition tPosOffset, uint8_t uMaxRef, AL_TPictMngrRefBuffers const* pPictMngrRefBuffers, AL_TDecBuffers* pPicBuffers);
