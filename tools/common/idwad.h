#pragma once

#include "psxtypes.h"

#pragma pack(push, 1)

#define WAD_IDENT "WAD2"
#define WAD_MAX_LUMPNAME 16

typedef struct {
  char ident[4];
  s32 numlumps;
  s32 infotableofs;
} qwadinfo_t;

typedef struct {
  s32 filepos;
  s32 disksize;
  s32 size;
  s8  type;
  s8  compression;
  s8  pad1, pad2;
  char name[WAD_MAX_LUMPNAME];
} qlumpinfo_t;

typedef struct {
  s32 width, height;
  u8 data[];
} qpic_t;

#pragma pack(pop)
