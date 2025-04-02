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
static u16 scr_tpage = 0;

static char scr_centermsg[MAX_SCR_LINE];
static s32 scr_centermsg_len = 0;
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

void Scr_Init(void) {
  // can't live without conchars
  pic_conchars = Spr_GetPic(PICID_CONCHARS);
  ASSERT(pic_conchars);

  // the rest should be sequential
  pic_bignumbers = Spr_GetPic(PICID_NUM_0);

  Sbar_Init();
}

static void Scr_DrawDebug(const int debug_mode) {
  player_state_t *p = gs.player;

  Scr_DrawText(2, 2, C_WHITE, VA("X=%04d Y=%04d Z=%04d",
    rs.vieworg.x>>12,
    rs.vieworg.y>>12,
    rs.vieworg.z>>12
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

void Scr_DrawScreen(const int debug_mode) {
  scr_tpage = 0;

  if (debug_mode)
    Scr_DrawDebug(debug_mode);

  Sbar_Draw(&gs.player[0]);

  if (!debug_mode && scr_msg_time > rs.frametime) {
    const s16 ofsx = 4;
    const s16 ofsy = 4;
    Scr_DrawText(ofsx, ofsy, C_WHITE, scr_msg);
  }

  if (scr_centermsg_time > rs.frametime) {
    const s16 ofsx = VID_CENTER_X - (FNT_SMALL_W / 2) * scr_centermsg_len;
    const s16 ofsy = VID_CENTER_Y - (FNT_SMALL_H / 2);
    Scr_DrawText(ofsx, ofsy, C_WHITE, scr_centermsg);
  }

  if (scr_flash_time > rs.frametime) {
    x32 t = scr_flash_time - rs.frametime;
    if (t > ONE) t = ONE;
    const u8 r = (t * scr_flash_color[0]) >> FIXSHIFT;
    const u8 g = (t * scr_flash_color[1]) >> FIXSHIFT;
    const u8 b = (t * scr_flash_color[2]) >> FIXSHIFT;
    Scr_DrawBlend(r, g, b);
  }
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

void Scr_DrawBlend(const u8 r, const u8 g, const u8 b) {
  SetTPage(getTPage(1, 1, VRAM_TEX_XSTART, VRAM_TEX_YSTART));
  TILE *prim = (TILE *)gpu_ptr;
  setTile(prim);
  setSemiTrans(prim, 1);
  setRGB0(prim, r, g, b);
  setXY0(prim, 0, 0);
  setWH(prim, VID_WIDTH, VID_HEIGHT);
  R_AddScreenPrim(sizeof(*prim));
}

void Scr_SetTopMsg(const char *str) {
  strncpy(scr_msg, str, MAX_SCR_LINE);
  scr_msg[MAX_SCR_LINE - 1] = 0;
  scr_msg_time = rs.frametime + SCR_LINE_TIME;
  scr_msg_len = strlen(scr_msg);
}

void Scr_SetCenterMsg(const char *str) {
  strncpy(scr_centermsg, str, MAX_SCR_LINE);
  scr_centermsg[MAX_SCR_LINE - 1] = 0;
  scr_centermsg_time = rs.frametime + SCR_LINE_TIME;
  scr_centermsg_len = strlen(scr_centermsg);
}

void Scr_SetFlashBlend(const u32 color) {
  scr_flash_color[0] = color & 0x0000FFu;
  scr_flash_color[1] = (color & 0x00FF00u) >> 8;
  scr_flash_color[2] = (color & 0xFF0000u) >> 16;
  scr_flash_time = rs.frametime + SCR_FLASH_TIME;
}
