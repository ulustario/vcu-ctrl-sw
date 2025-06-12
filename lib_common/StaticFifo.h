// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_rtos/types.h"

/*****************************************************************************
  Thread-safe implementation for SINGLE producer/consumer if <int> writes are atomics
  Note that Empty() is head==tail, thus only total_elements-1 entries may be used.
*****************************************************************************/
typedef struct
{
  void** elements;
  int32_t head;
  int32_t tail;
  int32_t total_elements;
}StaticFifo;

bool StaticFifo_Init(StaticFifo* self, void* elements[], int32_t total_elements);

bool StaticFifo_Enqueue(StaticFifo* self, void* element);
void* StaticFifo_Dequeue(StaticFifo* self);
bool StaticFifo_Empty(StaticFifo const* self);

void* StaticFifo_Front(StaticFifo const* self);
int32_t StaticFifo_Size(StaticFifo const* self);
void* StaticFifo_At(StaticFifo const* self, int32_t iOffset);
bool StaticFifo_IsIn(StaticFifo const* self, void* element);
