#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "psxtypes.h"
#include "util.h"
#include "idpak.h"
#include "pak.h"

int pak_open(pak_t *pak, const char *fname) {
  FILE *f = fopen(fname, "rb");
  if (!f) return -1;

  fread(&pak->header, sizeof(pak->header), 1, f);
  if (pak->header.id != PACK_ID) { fclose(f); return -2; }

  pak->numfiles = pak->header.dirlen / sizeof(qpackfile_t);
  assert(pak->numfiles <= MAX_FILES_IN_PACK);

  fseek(f, pak->header.dirofs, SEEK_SET);
  fread(pak->files, sizeof(*pak->files), pak->numfiles, f);

  pak->fp = f;
  return 0;
}

void pak_close(pak_t *pak) {
  fclose(pak->fp);
  memset(pak, 0, sizeof(*pak));
}

u8 *pak_readfile(pak_t *pak, const char *fname, size_t *outsize) {
  for (int i = 0; i < pak->numfiles; ++i) {
    if (!strncmp(fname, pak->files[i].name, MAX_PACK_PATH)) {
      u8 *buf = malloc(pak->files[i].filelen);
      assert(buf);
      fseek(pak->fp, pak->files[i].filepos, SEEK_SET);
      fread(buf, pak->files[i].filelen, 1, pak->fp);
      if (outsize) *outsize = pak->files[i].filelen;
      return buf;
    }
  }
  return NULL;
}

const char *pak_findfile(pak_t *pak, const char *match, const char *prev) {
  if (!prev || pak->findptr >= MAX_FILES_IN_PACK)
    pak->findptr = 0;
  for (; pak->findptr < MAX_FILES_IN_PACK; ++pak->findptr) {
    if (strstr(pak->files[pak->findptr].name, match))
      return pak->files[pak->findptr++].name;
  }
  return NULL;
}
