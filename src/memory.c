#include <malloc.h>
#include <string.h>

#include "types.h"
#include "system.h"
#include "memory.h"

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define RAM_END 0x80200000

#define MEM_ALIGNMENT  8
#define MEM_MAX_ALLOCS 1024

static u8 *mem_base;
static u8 *mem_ptr;
static u8 *mem_lastptr;
static int mem_size;
static int mem_left;

static u32 mem_allocs[MEM_MAX_ALLOCS];
static u32 mem_numallocs;
static u32 mem_mark;

void Mem_Init(void) {
  // fuck the heap, but we'll leave some in case libc needs it
  SetHeapSize(MALLOC_HEAP_SIZE);

  // the 4 is for the malloc magic I think
  mem_base = (u8 *)GetBSSend() + 4 + MALLOC_HEAP_SIZE;
  mem_base = (u8 *)ALIGN((u32)mem_base, MEM_ALIGNMENT);
  mem_size = mem_left = (u8 *)RAM_END - mem_base - STACK_SIZE;
  mem_ptr = mem_lastptr = mem_base;
  mem_numallocs = 0;
  mem_mark = 0;

  Sys_Printf("Mem_Init: membase=%08x memsize=%08x\n", (u32)mem_base, mem_size);
}

void *Mem_Alloc(const u32 size) {
  const u32 asize = ALIGN(size, MEM_ALIGNMENT);
  if (size == 0)
    Sys_Error("Mem_Alloc: size == 0");
  if (mem_left < asize)
    Sys_Error("Mem_Alloc: failed to alloc %d bytes", size);
  if (mem_numallocs == MEM_MAX_ALLOCS)
    Sys_Error("Mem_Alloc: MAX_ALLOCS reached");
  mem_lastptr = mem_ptr;
  mem_ptr += asize;
  mem_left -= asize;
  mem_allocs[mem_numallocs++] = asize;
  memset(mem_lastptr, 0, asize);
  return mem_lastptr;
}

void Mem_Free(void *ptr) {
  if (!ptr)
    return; // NULL free is a no-op
  if (ptr != mem_lastptr)
    Sys_Error("Mem_Free: this is a stack allocator you dolt");
  if (mem_numallocs == 0)
    Sys_Error("Mem_Free: nothing to free");

  const u32 size = mem_allocs[--mem_numallocs];
  mem_left += size;
  mem_ptr = mem_lastptr;
  mem_lastptr -= size;
}

void Mem_SetMark(void) {
  if (mem_numallocs)
    mem_mark = mem_numallocs - 1;
}

void Mem_FreeToMark(void) {
  for (; mem_numallocs > mem_mark; ) {
    const u32 size = mem_allocs[--mem_numallocs];
    mem_left += size;
    mem_ptr = mem_lastptr;
    mem_lastptr -= size;
  }
}

u32 Mem_GetFreeSpace(void) {
  return mem_left;
}
