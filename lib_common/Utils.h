// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Allocator.h"

static const int32_t NUMCORE_AUTO = 0;
static const int32_t MAX_BIT_DEPTH_MINUS_8 = 4;
static const int32_t MAX_POC_LSB_MINUS_4 = 12;

#define ARRAY_SIZE(x) (int)(sizeof(x) / sizeof((x)[0]))

/***************************************************************************/
static inline size_t BitsToBytes(size_t zBits)
{
  return (zBits + 7) / 8;
}

/***************************************************************************/
static inline size_t BytesToBits(size_t zBytes)
{
  return zBytes * 8;
}

/***************************************************************************/
static inline int32_t Clip3(int32_t iVal, int32_t iMin, int32_t iMax)
{
  return (iVal < iMin) ? iMin : ((iVal > iMax) ? iMax : iVal);
}

/***************************************************************************/
static inline AL_64S Clip3ll(AL_64S iVal, AL_64S iMin, AL_64S iMax)
{
  return ((iVal) < (iMin)) ? (iMin) : ((iVal) > (iMax)) ? (iMax) : (iVal);
}

/***************************************************************************/
static inline int32_t Max(int32_t iVal1, int32_t iVal2)
{
  return (iVal1 < iVal2) ? iVal2 : iVal1;
}

/***************************************************************************/
static inline size_t UnsignedMax(size_t zVal1, size_t zVal2)
{
  return (zVal1 < zVal2) ? zVal2 : zVal1;
}

/***************************************************************************/
static inline size_t UnsignedMin(size_t iVal1, size_t iVal2)
{
  return (iVal1 > iVal2) ? iVal2 : iVal1;
}

/***************************************************************************/
static inline int32_t Min(int32_t iVal1, int32_t iVal2)
{
  return (iVal1 > iVal2) ? iVal2 : iVal1;
}

/***************************************************************************/
static inline int32_t Abs(int32_t iVal)
{
  return (iVal > 0) ? iVal : -iVal;
}

/***************************************************************************/
static inline int32_t Sign(int32_t iVal)
{
  return (iVal > 0) ? 1 : ((iVal < 0) ? -1 : 0);
}

/***************************************************************************/
static inline int32_t DivideRoundUp(int32_t iVal, int32_t iDiv)
{
  return iVal >= 0 ? ((iVal + iDiv - 1) / iDiv) : (iVal / iDiv);
}

/***************************************************************************/
static inline int32_t RoundUp(int32_t iVal, int32_t iRnd)
{
  return DivideRoundUp(iVal, iRnd) * iRnd;
}

/***************************************************************************/
static inline int32_t RoundDown(int32_t iVal, int32_t iRnd)
{
  return iVal >= 0 ? (iVal / iRnd) * iRnd : ((iVal - iRnd + 1) / iRnd) * iRnd;
}

/***************************************************************************/
static inline size_t UnsignedRoundUp(size_t zVal, size_t zRnd)
{
  return ((zVal + zRnd - 1) / zRnd) * zRnd;
}

/***************************************************************************/
static inline size_t UnsignedRoundDown(size_t zVal, size_t zRnd)
{
  return (zVal / zRnd) * zRnd;
}

/***************************************************************************/
int32_t ceil_log2(int32_t n);

/****************************************************************************/
int32_t floor_log2(int32_t n);

/****************************************************************************/
int32_t GetBlkNumber(AL_TDimension tDim, uint32_t uBlkWidth, uint32_t uBlkHeight);

/****************************************************************************/
static inline int32_t GetSquareBlkNumber(AL_TDimension tDim, uint32_t uBlkSize) { return GetBlkNumber(tDim, uBlkSize, uBlkSize); }

/****************************************************************************/
int16_t MaxInArray(const int16_t tab[], int32_t arraySize);

/****************************************************************************/
int16_t MinInArray(const int16_t tab[], int32_t arraySize);

/****************************************************************************/
bool IsWindowEmpty(AL_TWindow tWindow);

/****************************************************************************/
AL_HANDLE AlignedAlloc(AL_TAllocator* pAllocator, const char* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset);

/*****************************************************************************
   \brief Reference picture status
 ***************************************************************************/
typedef enum
{
  SHORT_TERM_REF,
  LONG_TERM_REF,
  UNUSED_FOR_REF,
  NON_EXISTING_REF,
  AL_MARKING_REF_MAX_ENUM, /* sentinel */
}AL_EMarkingRef;
