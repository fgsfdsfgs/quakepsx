#pragma once

#include <stdlib.h>
#include "psxtypes.h"
#include "idwad.h"

typedef struct {
  u32 size;
  qwadinfo_t *info;
  qlumpinfo_t *lumps;
  u8 *data;
} wad_t;

int wad_open(wad_t *wad, u8 *data, const size_t size);
void wad_close(wad_t *wad);
u8 *wad_get_lump(wad_t *wad, const char *lname, size_t *outsize);
u8 *wad_get_lump_by_num(wad_t *wad, const int lid, size_t *outsize);
