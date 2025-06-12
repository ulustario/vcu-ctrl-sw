// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Driver
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

typedef enum AL_EDriverError
{
  DRIVER_SUCCESS,
  DRIVER_ERROR_UNKNOWN,
  DRIVER_ERROR_NO_MEMORY,
  DRIVER_ERROR_CHANNEL,
  DRIVER_TIMEOUT,
}AL_EDriverError;

#define AL_POLL_MSG 0xfffffffc

/*****************************************************************************
    \brief Interfaces with a device.
    The device can either be the interface of a kernel driver like al5e, al5r or al5d
    or it could also be a socket, this is implementation dependant.
    \see AL_GetHardwareDriver for the kernel driver implementation
*****************************************************************************/
typedef struct AL_TDriver AL_TDriver;
typedef struct AL_TDriverVTable
{
  int32_t (* pfnOpen)(AL_TDriver* driver, const char* device);
  void (* pfnClose)(AL_TDriver* driver, int32_t fd);
  AL_EDriverError (* pfnPostMessage)(AL_TDriver* driver, int32_t fd, uint32_t messageId, void* data, bool isBlocking);
}AL_TDriverVTable;

struct AL_TDriver
{
  AL_TDriverVTable const* vtable;
};

static inline
int32_t AL_Driver_Open(AL_TDriver* driver, const char* device)
{
  return driver->vtable->pfnOpen(driver, device);
}

static inline
void AL_Driver_Close(AL_TDriver* driver, int32_t fd)
{
  driver->vtable->pfnClose(driver, fd);
}

static inline
AL_EDriverError AL_Driver_PostBlockingMessage(AL_TDriver* driver, int32_t fd, uint32_t messageId, void* data)
{
  return driver->vtable->pfnPostMessage(driver, fd, messageId, data, true);
}

static inline
AL_EDriverError AL_Driver_PostNonBlockingMessage(AL_TDriver* driver, int32_t fd, uint32_t messageId, void* data)
{
  return driver->vtable->pfnPostMessage(driver, fd, messageId, data, false);
}

/*!@}*/
