#include <string.h>

#include "common.h"
#include "game.h"
#include "render.h"

#define MAX_LIGHTVALUE 400

u16 r_lightstylevalue[MAX_LIGHTSTYLES + 1];

struct {
  char map[MAX_STYLESTRING];
  u16 len;
} r_lightstyle[MAX_USERSTYLES];

void R_SetLightStyle(const int i, const char *map) {
  if (i < MAX_USERSTYLES) {
    // proper lightstyle string
    strncpy(r_lightstyle[i].map, map, MAX_STYLESTRING - 1);
    r_lightstyle[i].len = strlen(map);
    for (u16 j = 0; j < r_lightstyle[i].len; ++j)
      r_lightstyle[i].map[j] -= 'a';
  } else if (i < MAX_LIGHTSTYLES) {
    // toggleable light; set value directly
    const u16 k = (u16)(map[0] - 'a') * 22;
    r_lightstylevalue[i] = k > MAX_LIGHTVALUE ? MAX_LIGHTVALUE : k;
  }
}

void R_InitLightStyles(void) {
  // normal
  R_SetLightStyle(0, "m");
  // 1 FLICKER (first variety)
  R_SetLightStyle(1, "mmnmmommommnonmmonqnmmo");
  // 2 SLOW STRONG PULSE
  R_SetLightStyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
  // 3 CANDLE (first variety)
  R_SetLightStyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
  // 4 FAST STROBE
  R_SetLightStyle(4, "mamamamamama");
  // 5 GENTLE PULSE 1
  R_SetLightStyle(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
  // 6 FLICKER (second variety)
  R_SetLightStyle(6, "nmonqnmomnmomomno");
  // 7 CANDLE (second variety)
  R_SetLightStyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");
  // 8 CANDLE (third variety)
  R_SetLightStyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
  // 9 SLOW STROBE (fourth variety)
  R_SetLightStyle(9, "aaaaaaaazzzzzzzz");
  // 10 FLUORESCENT FLICKER
  R_SetLightStyle(10, "mmamammmmammamamaaamammma");
  // 11 SLOW PULSE NOT FADE TO BLACK
  R_SetLightStyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

  // dummy style
  r_lightstylevalue[MAX_LIGHTSTYLES] = 0;
}

void R_UpdateLightStyles(const x32 time) {
  register const int i = (time * 10) >> FIXSHIFT;
  register u16 k;
  for (register int j = 0; j < MAX_USERSTYLES; ++j) {
    if (!r_lightstyle[j].len) {
      r_lightstylevalue[j] = 256;
      continue;
    }
    k = i % r_lightstyle[j].len;
    k = r_lightstyle[j].map[k] * 22;
    r_lightstylevalue[j] = k > MAX_LIGHTVALUE ? MAX_LIGHTVALUE : k;
  }
}

void R_LightEntity(edict_t *ent) {
  if (ent->v.effects & EF_MUZZLEFLASH) {
    const u16 boost = ent->v.light + 0x80;
    ent->v.light = boost > 0xFF ? 0xFF : boost;
    ent->v.effects &= ~EF_MUZZLEFLASH; // FIXME: clear this for all entities
    return;
  }

  ent->v.light = 0;

  if (!ent->num_leafs)
    return;

  u32 avglight = 0;
  for (int i = 0; i < ent->num_leafs; ++i) {
    const mleaf_t *leaf = gs.worldmodel->leafs + ent->leafnums[i] + 1;
    const u32 light =
      leaf->lightmap[0] * r_lightstylevalue[leaf->styles[0]] +
      leaf->lightmap[1] * r_lightstylevalue[leaf->styles[1]];
    avglight += light >> 8;
  }

  avglight /= ent->num_leafs;
  ent->v.light = avglight > 0xff ? 0xff : avglight;
}
