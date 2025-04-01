#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "psxtypes.h"
#include "util.h"
#include "idwad.h"
#include "wad.h"

int wad_open(wad_t *wad, u8 *data, const size_t size) {
  qwadinfo_t *header = (qwadinfo_t *)data;

  if (size < sizeof(*header) || memcmp(header->ident, WAD_IDENT, sizeof(header->ident)) != 0)
    return -1;

  assert(size >= header->infotableofs + header->numlumps * sizeof(qlumpinfo_t));

  wad->info = header;
  wad->lumps = (qlumpinfo_t *)(data + header->infotableofs);
  wad->data = data;
  wad->size = size;

  return 0;
}

void wad_close(wad_t *wad) {
  wad->data = NULL;
  wad->size = 0;
  wad->lumps = NULL;
  free(wad->info);
  wad->info = NULL;
}

u8 *wad_get_lump(wad_t *wad, const char *lname, size_t *outsize) {
  if (!wad || !wad->info || !wad->lumps || !wad->size || !wad->info->numlumps)
    return NULL;

  char upper[WAD_MAX_LUMPNAME + 1];
  int i;
  for (i = 0; i < WAD_MAX_LUMPNAME && lname[i]; ++i)
    upper[i] = toupper(lname[i]);
  upper[i] = '\0';

  for (s32 i = 0; i < wad->info->numlumps; ++i) {
    if (!strncmp(wad->lumps[i].name, upper, WAD_MAX_LUMPNAME)) {
      if (outsize) *outsize = wad->lumps[i].size;
      return wad->data + wad->lumps[i].filepos;
    }
  }

  return NULL;
}

u8 *wad_get_lump_by_num(wad_t *wad, const int lid, size_t *outsize) {
  if (!wad || !wad->info || !wad->lumps || !wad->size || !wad->info->numlumps || lid < 0 || lid >= wad->info->numlumps)
    return NULL;
  if (outsize) *outsize = wad->lumps[lid].size;
  return wad->data + wad->lumps[lid].filepos;
}
