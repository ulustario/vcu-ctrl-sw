// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_rtos
   !@{
   \file
******************************************************************************/
#pragma once

#include <stddef.h> // for NULL and size_t
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#define AL_INTROSPECT(...)

#ifdef __GNUC__
#include <stdalign.h>

#define _CRT_SECURE_NO_WARNINGS
#define AL_DEPRECATED(msg) __attribute__((deprecated(msg)))
#define __AL_ALIGNED__(x) __attribute__((__aligned__(x)))

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#else // _MSC_VER

#define __attribute__(x)
#define __AL_ALIGNED__(x) __declspec(align(x))
#define AL_DEPRECATED(msg) __declspec(deprecated(msg))

#ifndef __cplusplus
#define static_assert(assertion, ...) _STATIC_ASSERT(assertion)
#endif

#endif

#define AL_DEPRECATED_ENUM_VALUE(eType, name, val, msg) AL_DEPRECATED(msg) static const eType name = val

typedef uint64_t __AL_ALIGNED__ (8) AL_64U; /*!< Ensure that unsigned 64-bit has same alignment on all platforms (especially 32-bit platforms) */
typedef int64_t __AL_ALIGNED__ (8) AL_64S; /*!< Ensure that signed 64-bit has same alignment on all platforms (especially 32-bit platforms) */
typedef uint8_t* AL_VADDR; /*!< Virtual address byte pointer */

typedef uint32_t AL_PADDR; /*!< Physical address, 32-bit address registers */
#define AL_PRINT_PADDR "%08" PRIX32 /*!< Use to print PADDR */

typedef AL_64U AL_PTR64;
typedef void* AL_HANDLE; /*!< Handle type */
typedef void const* AL_CONST_HANDLE; /*!< Handle const type */

/*!@}*/
