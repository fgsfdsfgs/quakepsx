#include "memory.h"

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "system.h"

#define RAM_END 0x80200000

#define MEM_ALIGNMENT 8
#define MEM_MAX_ALLOCS 256

static u8 *mem_base;
static u8 *mem_ptr;
static u8 *mem_lastptr;
static int mem_size;
static int mem_left;

static u32 mem_allocs[MEM_MAX_ALLOCS];
static u32 mem_numallocs;

static struct {
  u8 *mem_ptr;
  u8 *mem_lastptr;
  s32 mem_left;
  u32 mem_numallocs;
} mem_mark[MEM_MARK_COUNT];

// this should be defined somewhere
extern u8 _end[];

void Mem_Init(void) {
  // shrink libc heap to acceptable levels; we still probably need it for some things
  InitHeap(_end + 4, MALLOC_HEAP_SIZE - 4);

  mem_base = _end + MALLOC_HEAP_SIZE;
  mem_base = (u8 *)ALIGN((u32)mem_base, MEM_ALIGNMENT);
  mem_size = mem_left = (u8 *)RAM_END - mem_base - STACK_SIZE;
  mem_ptr = mem_lastptr = mem_base;
  mem_numallocs = 0;

  printf("Mem_Init: mem_base=%08x mem_size=%u\n", (u32)mem_base, mem_size);
}

void *Mem_Alloc(const u32 size) {
  const u32 asize = ALIGN(size, MEM_ALIGNMENT);
  if (size == 0)
    Sys_Error("Mem_Alloc: size == 0");
  if (mem_left < asize)
    Sys_Error("Mem_Alloc: failed to alloc %u bytes", size);
  if (mem_numallocs == MEM_MAX_ALLOCS)
    Sys_Error("Mem_Alloc: MAX_ALLOCS reached");
  mem_lastptr = mem_ptr;
  mem_ptr += asize;
  mem_left -= asize;
  mem_allocs[mem_numallocs++] = asize;
  return mem_lastptr;
}

void *Mem_ZeroAlloc(const u32 size) {
  void *buf = Mem_Alloc(size);
  memset(buf, 0, size);
  return buf;
}

void *Mem_Realloc(void *ptr, const u32 newsize) {
  if (!ptr)
    return Mem_Alloc(newsize); // just like realloc()
  const u32 anewsize = ALIGN(newsize, MEM_ALIGNMENT);
  const u32 oldsize = mem_allocs[mem_numallocs - 1];
  if (ptr != mem_lastptr || !mem_numallocs)
    Sys_Error("Mem_Realloc: this is a stack allocator you dolt");
  if (mem_left < anewsize)
    Sys_Error("Mem_Realloc: failed to realloc %p from %u to %u bytes", oldsize, anewsize);
  mem_left -= (int)newsize - (int)oldsize;
  mem_ptr = mem_lastptr + newsize;
  mem_allocs[mem_numallocs - 1] = newsize;
  return mem_lastptr;
}

void Mem_Free(void *ptr) {
  if (!ptr)
    return;  // NULL free is a no-op
  if (ptr != mem_lastptr)
    Sys_Error("Mem_Free: this is a stack allocator you dolt");
  if (mem_numallocs == 0)
    Sys_Error("Mem_Free: nothing to free");
  const u32 size = mem_allocs[--mem_numallocs];
  mem_left += size;
  mem_ptr = mem_lastptr;
  if (mem_numallocs)
    mem_lastptr -= mem_allocs[mem_numallocs - 1];
  else
    ASSERT(mem_lastptr == mem_base);
}

void Mem_SetMark(const int m) {
  mem_mark[m].mem_numallocs = mem_numallocs;
  mem_mark[m].mem_left = mem_left;
  mem_mark[m].mem_lastptr = mem_lastptr;
  mem_mark[m].mem_ptr = mem_ptr;
  printf("Mem_SetMark: mark %d set at %p/%p, %d left\n", m, mem_lastptr, mem_ptr, mem_left);
}

void Mem_FreeToMark(const int m) {
  mem_numallocs = mem_mark[m].mem_numallocs;
  mem_left = mem_mark[m].mem_left;
  mem_lastptr = mem_mark[m].mem_lastptr;
  mem_ptr = mem_mark[m].mem_ptr;
  printf("Mem_FreeToMark: reset to mark %d: %p/%p, %d left\n", m, mem_lastptr, mem_ptr, mem_left);
}

u32 Mem_GetFreeSpace(void) {
  return mem_left;
}
