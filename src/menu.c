#include "common.h"
#include "input.h"
#include "screen.h"
#include "progs.h"
#include "menu.h"

static const pic_t *pic_quake;

static menu_t *menu_current = NULL;

void Menu_Toggle(void) {
  if (menu_current) {
    IN_Clear();
    menu_current = NULL;
    return;
  }

  menu_current = &menu_main;

  pic_quake = Spr_GetPic(PICID_QPLAQUE);
}

void Menu_Close(void) {
  menu_current = NULL;
  IN_Clear();
}

void Menu_Update(void) {
  if (!menu_current)
    return;

  if (IN_ButtonPressed(PAD_DOWN)) {
    s32 i = menu_current->selection;
    do {
      i++;
      if (i >= menu_current->numoptions)
        i = 0;
    } while (menu_current->options[i].type == OPT_LABEL);
    menu_current->selection = i;
  } else if (IN_ButtonPressed(PAD_UP)) {
    s32 i = menu_current->selection;
    do {
      i--;
      if (i < 0)
        i = menu_current->numoptions - 1;
    } while (menu_current->options[i].type == OPT_LABEL);
    menu_current->selection = i;
  }

  if (IN_ButtonPressed(PAD_CIRCLE)) {
    menu_current = menu_current->prev;
    return;
  }

  menuoption_t *opt = menu_current->options + menu_current->selection;

  const qboolean select = IN_ButtonPressed(PAD_CROSS);
  const qboolean left = IN_ButtonPressed(PAD_LEFT);
  const qboolean right = IN_ButtonPressed(PAD_RIGHT);

  switch (opt->type) {
  case OPT_BUTTON:
    if (select) {
      if (opt->callback)
        opt->callback(opt);
      if (opt->button.nextmenu) {
        menu_t *prev = menu_current;
        menu_current = opt->button.nextmenu;
        menu_current->prev = prev;
        return;
      }
    }
    break;

  case OPT_CHOICE:
    if (select || right) {
      *(s32 *)opt->value += 1;
      if (*(s32 *)opt->value >= opt->choice.numchoices)
        *(s32 *)opt->value = 0;
    } else if (left) {
      *(s32 *)opt->value -= 1;
      if (*(s32 *)opt->value < 0)
        *(s32 *)opt->value = opt->choice.numchoices - 1;
    }
    if (select || right || left) {
      if (opt->callback)
        opt->callback(opt);
    }
    break;

  case OPT_SLIDER:
    if (select || right) {
      *(s32 *)opt->value += opt->slider.step;
      if (*(s32 *)opt->value > opt->slider.max)
        *(s32 *)opt->value = opt->slider.max;
    } else if (left) {
      *(s32 *)opt->value -= opt->slider.step;
      if (*(s32 *)opt->value < opt->slider.min)
        *(s32 *)opt->value = opt->slider.min;
    }
    if (select || right || left) {
      if (opt->callback)
        opt->callback(opt);
    }
    break;
  }
}

static void DrawOption(s32 x, s32 y, const menuoption_t *opt, const qboolean selected) {
  if (selected)
    Scr_DrawText(x - 10, y, C_WHITE, "\x0d");

  Scr_DrawTextOffset(opt->type == OPT_LABEL ? 128 : 0, x, y, C_WHITE, opt->title);

  x += 104;

  s32 v = 0;

  switch (opt->type) {
  case OPT_CHOICE:
    v = *(s32 *)opt->value;
    Scr_DrawTextOffset(128, x, y, C_WHITE, opt->choice.choices[v]);
    break;
  case OPT_SLIDER:
    v = *(s32 *)opt->value;
    Scr_DrawText(x, y, C_WHITE, "\x80\x81\x81\x81\x81\x81\x81\x81\x81\x81\x82");
    x += 8 + (v / opt->slider.step) * 8;
    Scr_DrawText(x, y, C_WHITE, "\x83");
    break;
  }
}

void Menu_Draw(void) {
  if (!menu_current)
    return;

  Scr_DrawBlendHalf(0, 0, 0);
  Scr_DrawPic(40, 32, C_WHITE, pic_quake);

  s32 basex = 40 + 16 + pic_quake->size.u;
  s32 basey = 32;
  Scr_DrawTextOffset(128, basex, basey, C_WHITE, menu_current->title);
  basey += 16;

  for (int i = 0; i < menu_current->numoptions; ++i) {
    const menuoption_t *opt = menu_current->options + i;
    DrawOption(basex, basey, opt, i == menu_current->selection);
    basey += 8;
  }
}

qboolean Menu_IsOpen(void) {
  return menu_current != NULL;
}
