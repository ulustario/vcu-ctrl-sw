// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common_dec/I_Feeder.h"

struct AL_TNullBufferFeeder
{
  AL_TFeederVtable const* vtable;
};

static void destroy(AL_TFeeder* hFeeder)
{
  Rtos_Free(hFeeder);
}

static bool pushBuffer(AL_TFeeder* hFeeder, AL_TBuffer* pBuf, size_t uSize, bool bLastBuffer)
{
  (void)hFeeder, (void)pBuf, (void)uSize, (void)bLastBuffer;
  return true;
}

static void signal(AL_TFeeder* hFeeder)
{
  (void)hFeeder;
}

static void flush(AL_TFeeder* hFeeder)
{
  (void)hFeeder;
}

static void reset(AL_TFeeder* pFeeder)
{
  (void)pFeeder;
}

static void freeBuf(AL_TFeeder* hFeeder, AL_TBuffer* pBuf)
{
  (void)hFeeder, (void)pBuf;
}

static const AL_TFeederVtable NullBufferFeederTable =
{
  &destroy,
  &pushBuffer,
  &signal,
  &flush,
  &reset,
  &freeBuf,
};

static inline AL_TFeeder* AL_NullBufferFeeder_Create(void)
{
  struct AL_TNullBufferFeeder* self = Rtos_Malloc(sizeof(*self));

  if(!self)
    return NULL;

  self->vtable = &NullBufferFeederTable;

  return (AL_TFeeder*)self;
}
