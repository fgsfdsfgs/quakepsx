#include "prcommon.h"

#define ICON_W 24
#define ICON_H 24
#define KEY_W  16
#define KEY_H  16
#define RUNE_W 8
#define RUNE_H 8

#define PAD_SIZE 4
#define LEFT_X (PAD_SIZE)
#define RIGHT_X (VID_WIDTH - PAD_SIZE)
#define BOTTOM_ROW_Y (VID_HEIGHT - 1 * (PAD_SIZE + ICON_H))
#define TOP_ROW_Y (VID_HEIGHT - 2 * (PAD_SIZE + ICON_H))

static const pic_t *pic_faces = NULL;
static const pic_t *pic_faces_pain = NULL;
static const pic_t *pic_faces_power = NULL;
static const pic_t *pic_armor = NULL;
static const pic_t *pic_ammo = NULL;
static const pic_t *pic_keys = NULL;
static const pic_t *pic_runes = NULL;

static x32 face_pain_time = 0;

u32 sbar_xhair_color = C_WHITE;
qboolean sbar_xhair = false;

static inline void DrawCrosshair(void) {
  // just draw a little dotted + with 4 rects for now
  // top and bottom
  Scr_DrawRect(VID_CENTER_X - 1, VID_CENTER_Y - 2, 2, 1, sbar_xhair_color, true);
  Scr_DrawRect(VID_CENTER_X - 1, VID_CENTER_Y + 1, 2, 1, sbar_xhair_color, true);
  // left and right
  Scr_DrawRect(VID_CENTER_X - 2, VID_CENTER_Y - 1, 1, 2, sbar_xhair_color, true);
  Scr_DrawRect(VID_CENTER_X + 1, VID_CENTER_Y - 1, 1, 2, sbar_xhair_color, true);
}

static inline const pic_t *PickFace(const s16 health, const u32 items) {
  const pic_t *pic;

  if ((items & (IT_INVISIBILITY | IT_INVULNERABILITY)) == (IT_INVISIBILITY | IT_INVULNERABILITY)) {
    // special face both invulnerability and invisiblity
    pic = pic_faces_power + 3;
  } else if (items & IT_QUAD) {
    pic = pic_faces_power + 0;
  } else if (items & IT_INVISIBILITY) {
    pic = pic_faces_power + 1;
  } else if (items & IT_INVULNERABILITY) {
    pic = pic_faces_power + 2;
  } else {
    const s16 index = (health >= PLAYER_HEALTH) ? 4 : (health / (PLAYER_HEALTH / 4));
    pic = ((face_pain_time > rs.frametime) ? pic_faces_pain : pic_faces) + index;
  }

  return pic;
}

static inline void DrawCounter(s16 x, s16 y, const pic_t *pic, const s16 val, const s16 redval, const qboolean ralign) {
  const u32 color = val <= redval ? C_RED : C_WHITE;
  if (ralign) {
    Scr_DrawPic(x, y, C_WHITE, pic); x -= FNT_BIG_W * 3 + 2;
    Scr_DrawDigits(x, y,color, VA("%3d", val));
  } else {
    Scr_DrawPic(x, y, C_WHITE, pic); x += ICON_W + 2;
    Scr_DrawDigits(x, y,color, VA("%d", val));
  }
}

void Sbar_Init(void) {
  pic_faces = Spr_GetPic(PICID_FACE5);
  pic_faces_pain = Spr_GetPic(PICID_FACE_P5);
  pic_faces_power = Spr_GetPic(PICID_FACE_QUAD);
  pic_armor = Spr_GetPic(PICID_SB_ARMOR1);
  pic_ammo = Spr_GetPic(PICID_SB_SHELLS);
  pic_keys = Spr_GetPic(PICID_SB_KEY1);
  pic_runes = Spr_GetPic(PICID_SB_SIGIL1);
}

void Sbar_DrawIntermission(const player_state_t *p) {
  const s32 min = (pr.completion_time >> FIXSHIFT) / 60;
  const s32 sec = (pr.completion_time >> FIXSHIFT) % 60;

  s16 y = VID_CENTER_Y - 24;
  Scr_DrawText(VID_CENTER_X - 28, y, C_YELLOW, "COMPLETED"); y += 10;
  Scr_DrawText(VID_CENTER_X - 44, y, C_WHITE, VA("Time:    %02d:%02d", min, sec)); y += 8;
  Scr_DrawText(VID_CENTER_X - 44, y, C_WHITE, VA("Secrets: %d/%d", pr.found_secrets, pr.total_secrets)); y += 8;
  Scr_DrawText(VID_CENTER_X - 44, y, C_WHITE, VA("Kills:   %d/%d", pr.killed_monsters, pr.total_monsters)); y += 8;
}

void Sbar_Draw(const player_state_t *p) {
  const pic_t *pic;

  if (pr.intermission_state == 1) {
    // draw level completion screen
    Sbar_DrawIntermission(p);
    return;
  }

  if (p->ent->v.health < 0)
    return; // we're fucking dead

  // bottom row
  DrawCounter(LEFT_X, BOTTOM_ROW_Y, PickFace(p->ent->v.health, p->stats.items), p->ent->v.health, 25, false);
  if (p->stats.weaponnum && p->stats.weaponnum != IT_AXE) {
    pic = pic_ammo + p->stats.ammonum;
    DrawCounter(RIGHT_X - ICON_W, BOTTOM_ROW_Y, pic, p->stats.ammo[p->stats.ammonum], 10, true);
  }

  // top row
  if (p->stats.items & IT_ARMOR3)
    pic = pic_armor + 2;
  else if (p->stats.items & IT_ARMOR2)
    pic = pic_armor + 1;
  else
    pic = pic_armor + 0;
  DrawCounter(LEFT_X, TOP_ROW_Y, pic, p->stats.armor, 25, false);

  // keys and shit
  s16 x = RIGHT_X - KEY_W;
  if (p->stats.items & IT_KEY1) {
    Scr_DrawPic(x, BOTTOM_ROW_Y - PAD_SIZE - KEY_H, C_WHITE, pic_keys + 0); x-= KEY_W;
  }
  if (p->stats.items & IT_KEY2) {
    Scr_DrawPic(x, BOTTOM_ROW_Y - PAD_SIZE - KEY_H, C_WHITE, pic_keys + 1); x -= KEY_W;
  }

  if (sbar_xhair)
    DrawCrosshair();
}

void Sbar_IndicateDamage(const s16 damage) {
  face_pain_time = rs.frametime + TO_FIX32(1);
  Scr_SetBlend(C_RED, SCR_FLASH_TIME);
}
