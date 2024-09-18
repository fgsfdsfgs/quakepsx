#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

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
    int sz = ftell(f);
    fseek(f, 0, SEEK_SET);
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

void img_unpalettize(const u8 *src, u8 *dst, const int w, const int h, const u8 *pal) {
  const u8 *ps = src;
  u8 *pd = dst;
  for (int i = 0; i < w * h; ++i, ++ps, pd += 3) {
    const int c = *ps;
    pd[0] = pal[3 * c + 0];
    pd[1] = pal[3 * c + 1];
    pd[2] = pal[3 * c + 2];
  }
}

u8 img_closest_color(const u8 rgb[3], const u8 *pal, const int palsiz) {
  double mindist = 256.0 * 256.0 * 256.0;
  int mincol = 0;
  for (int i = 0; i < palsiz; ++i) {
    const u8 *prgb = pal + i * 3;
    if (rgb[0] == prgb[0] && rgb[1] == prgb[1] && rgb[2] == prgb[2])
      return i;
    const double dc[3] = {
      (double)rgb[0] - (double)prgb[0],
      (double)rgb[1] - (double)prgb[1],
      (double)rgb[2] - (double)prgb[2],
    };
    const double dist = sqrt(dc[0] * dc[0] + dc[1] * dc[1] + dc[2] * dc[2]);
    if (dist < mindist) {
      mindist = dist;
      mincol = i;
    }
  }
  return mincol;
}

void img_palettize(const u8 *src, u8 *dst, const int w, const int h, const u8 *pal, const int palsiz) {
  const u8 *ps = src;
  u8 *pd = dst;
  for (int i = 0; i < w * h; ++i, ++pd, ps += 3) {
    const int c = img_closest_color(ps, pal, palsiz);
    *pd = c;
  }
}

int img_quantize(const u8 *src, u8 *dst, const int w, const int h, u8 *outpal) {
  const u8 *ps = src;
  u8 *pd = dst;
  int numc = 0;
  for (int i = 0; i < w * h; ++i, ++pd, ps += 3) {
    int c;
    for (c = 0; c < numc; ++c)
      if (outpal[c * 3 + 0] == ps[0] && outpal[c * 3 + 1] == ps[1] && outpal[c * 3 + 2] == ps[2])
        break;
    if (c == numc) {
      outpal[c * 3 + 0] = ps[0];
      outpal[c * 3 + 1] = ps[1];
      outpal[c * 3 + 2] = ps[2];
      ++numc;
    }
  }
  return numc;
}

// COM_Parse from Quake
const char *com_parse(const char *data, char *com_token)
{
  int c;
  int len;

  len = 0;
  com_token[0] = 0;

  if (!data)
    return NULL;

  // skip whitespace
skipwhite:
  while ( (c = *data) <= ' ')
  {
    if (c == 0)
      return NULL; // end of file;
    data++;
  }

  // skip // comments
  if (c=='/' && data[1] == '/')
  {
    while (*data && *data != '\n')
      data++;
    goto skipwhite;
  }
  

  // handle quoted strings specially
  if (c == '\"')
  {
    data++;
    while (1)
    {
      c = *data++;
      if (c=='\"' || !c)
      {
        com_token[len] = 0;
        return data;
      }
      com_token[len] = c;
      len++;
    }
  }

  // parse single characters
  if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
  {
    com_token[len] = c;
    len++;
    com_token[len] = 0;
    return data+1;
  }

  // parse a regular word
  do
  {
    com_token[len] = c;
    data++;
    len++;
    c = *data;
    if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
        break;
  } while (c>32);

  com_token[len] = 0;
  return data;
}
