#include "common.h"
#include "render.h"
#include "input.h"
#include "screen.h"
#include "sound.h"
#include "progs.h"
#include "menu.h"

static const pic_t *pic_quake;

static s32 dbg_episode = 1;
static s32 dbg_map = 1;

s32 menu_open = 0;

void Menu_Toggle(void) {
  menu_open ^= 1;
  if (!menu_open)
    return;

  pic_quake = Spr_GetPic(PICID_QPLAQUE);
}

void Menu_Update(void) {
  if (IN_ButtonPressed(PAD_DOWN)) {
    dbg_map--;
    if (dbg_map <= 0)
      dbg_map = 8;
  } else if (IN_ButtonPressed(PAD_UP)) {
    dbg_map++;
    if (dbg_map >= 8)
      dbg_map = 1;
  }
  
  if (IN_ButtonPressed(PAD_LEFT)) {
    dbg_episode--;
    if (dbg_episode < 0)
      dbg_episode = 5;
  } else if (IN_ButtonPressed(PAD_RIGHT)) {
    dbg_episode++;
    if (dbg_episode > 5)
      dbg_episode = 0;
  }

  if (IN_ButtonPressed(PAD_TRIANGLE)) {
    Menu_Toggle();
    return;
  }

  if (IN_ButtonPressed(PAD_CROSS)) {
    char map[16];
    if (dbg_episode == 0)
      snprintf(map, sizeof(map), "%s", "START");
    else if (dbg_episode == 5)
      snprintf(map, sizeof(map), "%s", "END");
    else
      snprintf(map, sizeof(map), "E%dM%d", dbg_episode, dbg_map);

    Menu_Toggle();

    G_RequestMap(map);
  }
}

void Menu_Draw(void) {
  char map[64];

  if (dbg_episode == 0)
    snprintf(map, sizeof(map), "Select map:\n%s", "START");
  else if (dbg_episode == 5)
    snprintf(map, sizeof(map), "Select map:\n%s", "END");
  else
    snprintf(map, sizeof(map), "Select map:\nE%dM%d", dbg_episode, dbg_map);

  Scr_DrawBlendHalf(0, 0, 0);
  Scr_DrawPic(32, 16, C_WHITE, pic_quake);
  Scr_DrawText(32 + 8 + pic_quake->size.u, 16 + 8, C_WHITE, map);
}
