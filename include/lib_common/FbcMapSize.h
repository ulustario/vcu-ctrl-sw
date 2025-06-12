// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Planes.h"

uint16_t AL_GetFbcMapPitch(uint32_t uWidth, AL_EFbStorageMode eFBStorageMode, uint8_t uBitDepth, bool bIsLuma);
uint32_t AL_GetFbcMapSize(AL_TDimension tDimension, AL_TPicFormat const* pPicFormat, AL_EPlaneId ePlaneId);
