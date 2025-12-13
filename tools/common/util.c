#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "pak.h"
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
  int err;
  for (int i = 0; i <= 9; ++i) {
    err = pak_open(&pak, strfmt("%s/pak%d.pak", dir, i));
#ifndef _WIN32
    // on case-sensitive filesystems also try the full-caps pak name as used by the og game
    if (err != 0)
      err = pak_open(&pak, strfmt("%s/PAK%d.PAK", dir, i));
#endif
    if (err == 0) {
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
    int sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc(sz);
    assert(buf);
    fread(buf, sz, 1, f);
    fclose(f);
    if (outsize) *outsize = sz;
    return buf;
  }

  fprintf(stderr, "warning: could not find '%s' in '%s' or in cwd\n", fname, dir);

  // never gets here
  return NULL;
}

const char *strfmt(const char *fmt, ...) {
  static char buf[MAX_VA_BUF][MAX_VA_LEN];
  static int bufidx = 0;
  bufidx = (bufidx + 1) % MAX_VA_BUF;
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf[bufidx], sizeof(buf[bufidx]), fmt, args);
  va_end(args);
  return buf[bufidx];
}

void convert_palette(u16 *dstpal, const u8 *pal, const int numcolors) {
  for (int i = 0; i < numcolors - 1; ++i) {
    const u8 r = *pal++;
    const u8 g = *pal++;
    const u8 b = *pal++;
    dstpal[i] = PSXRGB(r, g, b);
    if (!dstpal[i])
      dstpal[i] = 0x8000; // replace with non-transparent black
  }
  // last color is transparent
  dstpal[numcolors - 1] = 0;
}
