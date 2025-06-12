// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Buffers
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferAPI.h"

/*****************************************************************************
  \brief QP Table location parameters
*****************************************************************************/
typedef struct AL_TQpTableLoc
{
  int32_t iChunkIdx;   /*!< Index of the chunk containing the QpTable */
  uint32_t uOffset;    /*!< Offset of the QpTable from beginning of the buffer chunk (in bytes) */
}AL_TQpTableLoc;

/*****************************************************************************
   \brief MetaData gathering
*****************************************************************************/
typedef struct AL_TQpTableMetaData
{
  AL_TMetaData tMeta;
  AL_TQpTableLoc tQpTable[3]; // B, P, I
}AL_TQpTableMetaData;

/*****************************************************************************
   \brief Create a QP table metadata.
   \return Returns NULL in case of failure. Returns a pointer to the metadata in
   case of success.
*****************************************************************************/
AL_TQpTableMetaData* AL_QpTableMetaData_Create(void);
/*!@}*/
