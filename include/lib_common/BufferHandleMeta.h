// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Buffers
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"

typedef struct AL_TInternalHandleMetaData AL_TInternalHandleMetaData;

typedef struct AL_THandleMetaData
{
  AL_TMetaData tMeta;
  AL_TInternalHandleMetaData* pInternal;
}AL_THandleMetaData;

/*****************************************************************************
   \brief Create a Handle metadata.
*****************************************************************************/
AL_THandleMetaData* AL_HandleMetaData_Create(int32_t iMaxHandles, int32_t iHandleSizeInBytes);
AL_THandleMetaData* AL_HandleMetaData_Clone(AL_THandleMetaData* pMeta);
bool AL_HandleMetaData_AddHandle(AL_THandleMetaData* pMeta, AL_HANDLE pHandle);
void AL_HandleMetaData_ResetHandles(AL_THandleMetaData* pMeta);
AL_HANDLE AL_HandleMetaData_GetHandle(AL_THandleMetaData const* pMeta, int32_t iNumHandle);
int32_t AL_HandleMetaData_GetNumHandles(AL_THandleMetaData const* pMeta);

/*!@}*/
