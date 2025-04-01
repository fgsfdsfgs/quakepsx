#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "../common/pak.h"
#include "../common/util.h"
#include "util.h"

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

int resmap_parse(const char *fname, char *list, const int max_num, const int entry_len, const int name_len) {
  int id = 0xFF;
  char line[MAX_TOKEN] = { 0 };

  FILE *f = fopen(fname, "rb");
  if (!f) {
    fprintf(stderr, "resmap_parse(): can't find resmap file %s\n", fname);
    return -1;
  }

  int max_index = -1;
  while (fgets(line, sizeof(line), f)) {
    char *p = strtok(line, " \t\r\n");
    if (!p || !isalnum(p[0]))
      continue;
    id = strtol(p, NULL, 16);
    if (id >= max_num)
      continue;
    p = strtok(NULL, " \t\r\n");
    if (p && p[0]) {
      if (id > max_index)
        max_index = id;
      strncpy(list + id * entry_len, p, name_len - 1);
    }
  }

  fclose(f);

  return max_index + 1;
}
