#pragma once

#include "psxtypes.h"

#define PACK_ID           0x4B434150U // "PACK"
#define MAX_PACK_PATH     56
#define MAX_FILES_IN_PACK 2048

#pragma pack(push, 1)

typedef struct {
  char name[MAX_PACK_PATH];
  s32 filepos;
  s32 filelen;
} qpackfile_t;

typedef struct {
  u32 id;
  s32 dirofs;
  s32 dirlen;
} qpackheader_t;

#pragma pack(pop)
