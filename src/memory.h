#pragma once

#include "types.h"

// we're rolling our own stack allocator, but we'll leave some space for malloc
// in case libc uses it or something

#define MALLOC_HEAP_SIZE 16384
#define STACK_SIZE       16384

void Mem_Init(void);
void *Mem_Alloc(const u32 size);
void Mem_Free(void *ptr);
void Mem_SetMark(void);
void Mem_FreeToMark(void);
u32 Mem_GetFreeSpace(void);
