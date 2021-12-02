#pragma once

#include "common.h"

// we're rolling our own stack allocator, but we'll leave some space for malloc
// in case libc uses it or something

#define MALLOC_HEAP_SIZE 16384
#define STACK_SIZE       16384

enum mem_mark {
  MEM_MARK_LO,
  MEM_MARK_HI,
  MEM_MARK_COUNT,
};

void Mem_Init(void);

// simple stack type allocator
void *Mem_Alloc(const u32 size);
void Mem_Free(void *ptr);

// same as `mem_alloc`, but zero-fills the allocated area
void *Mem_ZeroAlloc(const u32 size);

// if `ptr` points to the top allocation, extends/shrinks it to `newsize`
// if `ptr` is NULL, calls `mem_alloc(newsize)`
void *Mem_Realloc(void *ptr, const u32 newsize);

// marks current alloc position
// mark can be MEM_MARK_LO or MEM_MARK_HI
void Mem_SetMark(const int mark);

// frees everything allocated after the mark set by last MemSetMark call with the same mark
void Mem_FreeToMark(const int mark);

// returns number of allocatable bytes
u32 Mem_GetFreeSpace(void);
