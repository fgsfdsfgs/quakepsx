#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "render.h"
#include "game.h"
#include "progs.h"
#include "screen.h"

static const pic_t *pic_conchars;
static const pic_t *pic_bignumbers;
static const pic_t *pic_menudot;
static const pic_t *pic_loading;
static u16 scr_tpage = 0;

static char scr_centermsg[MAX_SCR_LINE];
static s32 scr_centermsg_ofsx = 0;
static s32 scr_centermsg_ofsy = 0;
static x32 scr_centermsg_time = 0;

static char scr_msg[MAX_SCR_LINE];
static s32 scr_msg_len = 0;
static x32 scr_msg_time = 0;

static u8 scr_flash_color[3];
static x32 scr_flash_time = 0;

static inline void SetTPage(const u16 v) {
  if (scr_tpage != v) {
    DR_TPAGE *tpage = (DR_TPAGE *)gpu_ptr;
    setDrawTPage(tpage, 0, 0, v);
    R_AddScreenPrim(sizeof(*tpage));
    scr_tpage = v;
  }
}

static inline void DrawPic8(const s16 x, const s16 y, const u8 du, const u8 dv, const u32 rgb, const pic_t *pic) {
  SPRT_8 *prim = (SPRT_8 *)gpu_ptr;
  *(u32 *)&prim->r0 = rgb;
  setSprt8(prim);
  setXY0(prim, x, y);
  setUV0(prim, pic->uv.u + du, pic->uv.v + dv);
  prim->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
  R_AddScreenPrim(sizeof(*prim));
}

static inline void DrawPic16(const s16 x, const s16 y, const u8 du, const u8 dv, const u32 rgb, const pic_t *pic) {
  SPRT_16 *prim = (SPRT_16 *)gpu_ptr;
  *(u32 *)&prim->r0 = rgb;
  setSprt16(prim);
  setXY0(prim, x, y);
  setUV0(prim, pic->uv.u + du, pic->uv.v + dv);
  prim->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
  R_AddScreenPrim(sizeof(*prim));
}

static inline void DrawPic(const s16 x, const s16 y, const u8 du, const u8 dv, const u32 rgb, const pic_t *pic) {
  SPRT *prim = (SPRT *)gpu_ptr;
  *(u32 *)&prim->r0 = rgb;
  setSprt(prim);
  setXY0(prim, x, y);
  setUV0(prim, pic->uv.u + du, pic->uv.v + dv);
  setWH(prim, pic->size.x, pic->size.y);
  prim->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
  R_AddScreenPrim(sizeof(*prim));
}

static inline void DrawBlend(const player_state_t *plr) {
  u16 r = 0;
  u16 g = 0;
  u16 b = 0;

  if (rs.viewleaf) {
    // if underwater, apply constant tint
    switch (rs.viewleaf->contents) {
    case CONTENTS_LAVA: r = 64; g = 20; b = 0; break;
    case CONTENTS_SLIME: r = 0; g = 13; b = 3; break;
    case CONTENTS_WATER: r = 32; g = 20; b = 12; break;
    default: break;
    }
  }

  if (scr_flash_time > rs.frametime) {
    // if a screen flash is currently happening, mix that in
    x32 t = scr_flash_time - rs.frametime;
    if (t > ONE) t = ONE;
    const u16 fr = (t * scr_flash_color[0]) >> FIXSHIFT;
    r += fr; if (r > 0x80) r = 0x80;
    const u16 fg = (t * scr_flash_color[1]) >> FIXSHIFT;
    g += fg; if (g > 0x80) g = 0x80;
    const u16 fb = (t * scr_flash_color[2]) >> FIXSHIFT;
    b += fb; if (b > 0x80) b = 0x80;
  }

  if (r | g | b)
    Scr_DrawBlendAdd(r, g, b);
}

static void DrawDebug(const int debug_mode) {
  player_state_t *p = gs.player;

  Scr_DrawText(2, 2, C_WHITE, VA("X=%04d Y=%04d Z=%04d",
    p->ent->v.origin.x>>12,
    p->ent->v.origin.y>>12,
    p->ent->v.origin.z>>12
  ));
  Scr_DrawText(2, 12, C_WHITE, VA("VX=%04d VY=%04d VZ=%04d",
    p->ent->v.velocity.x>>12,
    p->ent->v.velocity.y>>12,
    p->ent->v.velocity.z>>12
  ));
  Scr_DrawText(2, 22, C_WHITE, VA("RX=%05d RY=%05d",
    rs.viewangles.x, 
    rs.viewangles.y
  ));
}

void Scr_Init(void) {
  // can't live without conchars
  pic_conchars = Spr_GetPic(PICID_CONCHARS);
  ASSERT(pic_conchars);

  pic_loading = Spr_GetPic(PICID_DISC);

  // the rest should be sequential
  pic_bignumbers = Spr_GetPic(PICID_NUM_0);
  pic_menudot = Spr_GetPic(PICID_MENUDOT1);

  Sbar_Init();
}

void Scr_DrawScreen(const int debug_mode) {
  scr_tpage = 0;

  player_state_t *plr = &gs.player[0];

  DrawBlend(plr);

  if (debug_mode)
    DrawDebug(debug_mode);

  if (!debug_mode && scr_msg_time > rs.frametime) {
    const s16 ofsx = 4;
    const s16 ofsy = 4;
    Scr_DrawText(ofsx, ofsy, C_WHITE, scr_msg);
  }

  if (scr_centermsg_time > rs.frametime)
    Scr_DrawText(scr_centermsg_ofsx, scr_centermsg_ofsy, C_WHITE, scr_centermsg);

  Sbar_Draw(plr);
}

void Scr_DrawText(const s16 x, const s16 y, const u32 rgb, const char *str) {
  SetTPage(pic_conchars->tpage);

  s16 tx = x;
  s16 ty = y;
  u8 tu, tv;
  for (const u8 *p = (const u8 *)str; *p; ++p) {
    switch (*p) {
    case ' ':
      tx += FNT_SMALL_W;
      break;
    case '\n':
      tx = x;
      ty += FNT_SMALL_H;
      break;
    default:
      tu = (((*p) & 0xF) << 3);
      tv = (((*p) >> 4) << 3);
      DrawPic8(tx, ty, tu, tv, rgb, pic_conchars);
      tx += FNT_SMALL_W;
      break;
    }
  }
}

void Scr_DrawDigits(const s16 x, const s16 y, const u32 rgb, const char *str) {
  SetTPage(pic_bignumbers->tpage);

  s16 tx = x;
  for (const u8 *p = (const u8 *)str; *p; ++p, tx += FNT_BIG_W) {
    const pic_t *pic;
    switch (*p) {
    case ' ': continue;
    case '/': pic = pic_bignumbers + 10; break;
    case ':': pic = pic_bignumbers + 11; break;
    default: pic = pic_bignumbers + (*p - '0'); break;
    }
    DrawPic(tx, y, 0, 0, rgb, pic);
  }
}

void Scr_DrawPic(const s16 x, const s16 y, const u32 rgb, const pic_t *pic) {
  SetTPage(pic->tpage);
  DrawPic(x, y, 0, 0, rgb, pic);
}

void Scr_DrawRect(const s16 x, const s16 y, const s16 w, const s16 h, const u32 rgb, const u8 blend) {
  TILE *prim = (TILE *)gpu_ptr;
  *(u32 *)&prim->r0 = rgb;
  setTile(prim);
  setSemiTrans(prim, blend);
  setXY0(prim, x, y);
  setWH(prim, w, h);
  R_AddScreenPrim(sizeof(*prim));
}

static inline void DrawBlendCommon(const u8 r, const u8 g, const u8 b) {
  TILE *prim = (TILE *)gpu_ptr;
  setTile(prim);
  setSemiTrans(prim, 1);
  setRGB0(prim, r, g, b);
  setXY0(prim, 0, 0);
  setWH(prim, VID_WIDTH, VID_HEIGHT);
  R_AddScreenPrim(sizeof(*prim));
}

void Scr_DrawBlendAdd(const u8 r, const u8 g, const u8 b) {
  SetTPage(getTPage(1, 1, VRAM_TEX_XSTART, VRAM_TEX_YSTART));
  DrawBlendCommon(r, g, b);
}

void Scr_DrawBlendHalf(const u8 r, const u8 g, const u8 b) {
  SetTPage(getTPage(1, 0, VRAM_TEX_XSTART, VRAM_TEX_YSTART));
  DrawBlendCommon(r, g, b);
}

void Scr_SetTopMsg(const char *str) {
  strncpy(scr_msg, str, MAX_SCR_LINE);
  scr_msg[MAX_SCR_LINE - 1] = 0;
  scr_msg_time = rs.frametime + SCR_LINE_TIME;
  scr_msg_len = strlen(scr_msg);
  Sys_Printf("%s\n", str);
}

void Scr_SetCenterMsg(const char *str) {
  strncpy(scr_centermsg, str, MAX_SCR_LINE);
  scr_centermsg[MAX_SCR_LINE - 1] = 0;
  scr_centermsg_time = rs.frametime + SCR_LINE_TIME;

  char *newline = strchr(scr_centermsg, '\n');
  const int numchars = newline ? (newline - scr_centermsg) : strlen(scr_centermsg);
  scr_centermsg_ofsx = VID_CENTER_X - (FNT_SMALL_W / 2) * numchars;
  scr_centermsg_ofsy = VID_CENTER_Y - (FNT_SMALL_H / 2) * (1 + !!newline);

  Sys_Printf("%s\n", str);
}

void Scr_SetBlend(const u32 color, const x32 time) {
  scr_flash_color[0] = color & 0x0000FFu;
  scr_flash_color[1] = (color & 0x00FF00u) >> 8;
  scr_flash_color[2] = (color & 0xFF0000u) >> 16;
  scr_flash_time = rs.frametime + time;
}

void Scr_BeginLoading(void) {
  R_DrawBlitSync(pic_loading, VID_WIDTH - pic_loading->size.u - 16, 16);
}

void Scr_TickLoading(void) {

}

void Scr_EndLoading(void) {

}
