#pragma once

#include <stdio.h>
#include "psxtypes.h"
#include "idpak.h"

typedef struct {
  qpackheader_t header;
  int numfiles;
  qpackfile_t files[MAX_FILES_IN_PACK];
  FILE *fp;
  int findptr;
} pak_t;

int pak_open(pak_t *pak, const char *fname);
void pak_close(pak_t *pak);
u8 *pak_readfile(pak_t *pak, const char *fname, size_t *outsize);
const char *pak_findfile(pak_t *pak, const char *match, const char *prev);
