#include "prcommon.h"

static const pic_t *pic_faces = NULL;
static const pic_t *pic_faces_pain = NULL;
static const pic_t *pic_faces_power = NULL;
static const pic_t *pic_armor = NULL;
static const pic_t *pic_ammo = NULL;
static const pic_t *pic_keys = NULL;
static const pic_t *pic_runes = NULL;

u32 hud_crosshair_color = C_WHITE;
qboolean hud_crosshair = false;

void HUD_Init(void) {
  pic_faces = Spr_GetPic(PICID_FACE1);
  pic_faces_pain = Spr_GetPic(PICID_FACE_P1);
  pic_faces_power = Spr_GetPic(PICID_FACE_QUAD);
  pic_armor = Spr_GetPic(PICID_SB_ARMOR1);
  pic_ammo = Spr_GetPic(PICID_SB_SHELLS);
  pic_keys = Spr_GetPic(PICID_SB_KEY1);
  pic_runes = Spr_GetPic(PICID_SB_SIGIL1);
}

static inline void DrawCrosshair(void) {
  // just draw a little dotted + with 4 rects for now
  // top and bottom
  Scr_DrawRect(VID_CENTER_X - 1, VID_CENTER_Y - 2, 2, 1, hud_crosshair_color, true);
  Scr_DrawRect(VID_CENTER_X - 1, VID_CENTER_Y + 1, 2, 1, hud_crosshair_color, true);
  // left and right
  Scr_DrawRect(VID_CENTER_X - 2, VID_CENTER_Y - 1, 1, 2, hud_crosshair_color, true);
  Scr_DrawRect(VID_CENTER_X + 1, VID_CENTER_Y - 1, 1, 2, hud_crosshair_color, true);
}

void HUD_Draw(void) {
  // bottom row

  s16 x = 4;
  s16 y = VID_HEIGHT - 4 - ICON_H;

  Scr_DrawPic(x, y, C_WHITE, pic_faces); x += ICON_W;
  Scr_DrawDigits(x, y, C_WHITE, "999");

  x = VID_WIDTH - ICON_W - 4;

  Scr_DrawPic(x, y, C_WHITE, pic_ammo); x -= FNT_BIG_W * 3;
  Scr_DrawDigits(x, y, C_WHITE, "999");

  // top row

  x = 4;
  y -= 4 + ICON_H;

  Scr_DrawPic(x, y, C_WHITE, pic_armor); x += ICON_W;
  Scr_DrawDigits(x, y, C_WHITE, "999");

  if (hud_crosshair)
    DrawCrosshair();
}
