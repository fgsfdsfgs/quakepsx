#include <string.h>

#include "common.h"
#include "system.h"
#include "render.h"

u16 r_lightstylevalue[MAX_LIGHTSTYLES + 1];

struct {
  char map[MAX_STYLESTRING];
  u16 len;
} r_lightstyle[MAX_USERSTYLES];

void R_SetLightStyle(const int i, const char *map) {
  ASSERT(i < MAX_USERSTYLES);
  strncpy(r_lightstyle[i].map, map, MAX_STYLESTRING - 1);
  r_lightstyle[i].len = strlen(map);
  for (u16 j = 0; j < r_lightstyle[i].len; ++j)
    r_lightstyle[i].map[j] -= 'a';
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
    r_lightstylevalue[j] = k;
  }
}
