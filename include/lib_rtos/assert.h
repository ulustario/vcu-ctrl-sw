// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

/****************************************************************************/
/* Assert */
/****************************************************************************/

#if !defined(NDEBUG)

#define Rtos_Assert(bCondition) \
  do \
  { \
    Rtos_AssertWithMessage(bCondition, # bCondition, __FILE__, __LINE__); \
  } while(false)

/****************************************************************************/
/*** W i n 3 2  &  L i n u x  c o m m o n ***/
/****************************************************************************/
#if defined(_WIN32) || defined(__linux__)

#include <assert.h>
#include "lib_rtos/lib_rtos.h"

#define Rtos_AssertWithMessage(bCondition, sMsg, sFile, iLine) \
  do \
  { \
    if(!(bCondition)) \
      Rtos_LogWithoutLevel("[%s:%i] %s\n", sFile, iLine, sMsg); \
    assert(bCondition); \
  } while(false)

#else

#if __MICROBLAZE__

#include "McuSys.h"
#include "McuDebug.h"

#define Rtos_AssertWithMessage(bCondition, sMsg, sFile, iLine) \
  do \
  { \
    (void)sFile; \
    (void)iLine; \
    Mcu_Debug_Assert(bCondition, sMsg); \
  } while(false)

#else

/****************************************************************************/
/*** N o O p e r a t i n g S y s t e m ***/
/****************************************************************************/

void Rtos_AssertWithMessage(bool bCondition, char const* sMsg, char const* sFile, int32_t iLine);

#endif
#endif

#else // !defined(NDEBUG)

#if __GNUC__ > 12
#define ASSUME(cond) __attribute__((assume(cond)))
#elif __GNUC__ > 4 && __GNUC_MINOR__ > 5

do
{
  if(!(cond))
    __builtin_unreachable();
}
while(0)
#else
#define ASSUME(cond) (void)(cond)
#endif

#define Rtos_AssertWithMessage(bCondition, sMsg, sFile, iLine) \
  do \
  { \
    ASSUME(bCondition); \
    (void)(sMsg); \
    (void)(sFile); \
    (void)(iLine); \
  } while(false)

#define Rtos_Assert(bCondition) \
  (void)(bCondition);

#endif // !defined(NDEBUG)
