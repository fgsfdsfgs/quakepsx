#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../common/pak.h"
#include "../common/util.h"
#include "util.h"

void panic(const char *fmt, ...) {
  va_list args;
  fputs("fatal error: ", stderr);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fputs("\n", stderr);
  printf("fuck\n");
  exit(1);
}

void *lmp_read(const char *dir, const char *fname, size_t *outsize) {
  // scan pak files first
  pak_t pak;
  for (int i = 0; i <= 9; ++i) {
    if (pak_open(&pak, strfmt("%s/pak%d.pak", dir, i)) == 0) {
      void *data = pak_readfile(&pak, fname, outsize);
      pak_close(&pak);
      if (data) return data;
    }
  }

  // try raw file in mod directory and in working directory
  FILE *f = fopen(strfmt("%s/%s", dir, fname), "rb");
  if (!f) f = fopen(fname, "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    void *buf = malloc(sz);
    assert(buf);
    fread(buf, sz, 1, f);
    fclose(f);
    if (outsize) *outsize = sz;
    return buf;
  }

  panic("could not find '%s' in '%s' or in cwd", fname, dir);

  // never gets here
  return NULL;
}
