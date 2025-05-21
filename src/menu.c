#include "common.h"
#include "render.h"
#include "input.h"
#include "screen.h"
#include "sound.h"
#include "cd.h"
#include "progs.h"
#include "menu.h"

typedef struct option_s option_t;
typedef struct menu_s menu_t;
typedef void (*menu_fn_t)(option_t *self);

enum optiontype_e {
  OPT_LABEL,
  OPT_BUTTON,
  OPT_CHOICE,
  OPT_SLIDER
};

struct option_s {
  u32 type;
  const char *title;
  void *value;
  menu_fn_t callback;
  union {
    struct {
      menu_t *nextmenu;
    } button;
    struct {
      const char **choices;
      s32 numchoices;
    } choice;
    struct {
      s32 min;
      s32 max;
      s32 step;
    } slider;
    struct {
      const char *path;
      const char *init;
    } file;
  };
};

struct menu_s {
  const char *title;
  option_t *options;
  s32 numoptions;
  s32 selection;
  struct menu_s *prev;
};

static s32 dbg_skill = 1;
static s32 dbg_episode = 1;
static s32 dbg_map = 0;

static const char *choices_episodes[] = {
  "START", "E1", "E2", "E3", "E4", "END"
};

static const char *choices_map[] = {
  "M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8",
};

static const char *choices_off_on[] = {
  "Off", "On"
};

static const char *choices_skill[] = {
  "Easy", "Medium", "Hard", "Nightmare"
};

static void Menu_StartNewGame(option_t *self);
static void Menu_DbgStartGame(option_t *self);
static void Menu_DbgEpChanged(option_t *self);
static void Menu_CdVolChanged(option_t *self) { CD_SetAudioVolume(cd_volume); }
static void Menu_SndVolChanged(option_t *self) { Snd_SetVolume(snd_volume); }

static option_t options_options[] = {
  { OPT_SLIDER, "Sound Volume", &snd_volume, Menu_SndVolChanged, { .slider = { 0, 128, 16        } } },
  { OPT_SLIDER, "Music Volume", &cd_volume,  Menu_CdVolChanged,  { .slider = { 0, 128, 16        } } },
  { OPT_CHOICE, "Crosshair",    &sbar_xhair, NULL,               { .choice = { choices_off_on, 2 } } },
};

static menu_t menu_options = {
  "Options",
  options_options, 3,
  0, NULL
};

static option_t options_select_map[] = {
  { OPT_CHOICE, "Episode:", &dbg_episode, Menu_DbgEpChanged, { .choice = { choices_episodes, 6 } } },
  { OPT_CHOICE, "Map:",     &dbg_map,     NULL,              { .choice = { choices_map, 8      } } },
  { OPT_CHOICE, "Skill:",   &dbg_skill,   NULL,              { .choice = { choices_skill, 4    } } },
  { OPT_LABEL,  "",         NULL,         NULL,              {                                   } },
  { OPT_BUTTON, "Start",    NULL,         Menu_DbgStartGame, {                                   } },
};

static menu_t menu_select_map = {
  "Select map",
  options_select_map, 5,
  0, NULL
};

static option_t options_main[] = {
  { OPT_BUTTON, "New game",   NULL, Menu_StartNewGame, {                                } },
  { OPT_BUTTON, "Select map", NULL, NULL,              { .button = { &menu_select_map } } },
  { OPT_BUTTON, "Options",    NULL, NULL,              { .button = { &menu_options    } } },
};

static menu_t menu_main = {
  "Main",
  options_main, 3,
  0, NULL
};

static const pic_t *pic_quake;

static menu_t *menu_current = NULL;

void Menu_Toggle(void) {
  if (menu_current) {
    menu_current = NULL;
    return;
  }

  menu_current = &menu_main;

  pic_quake = Spr_GetPic(PICID_QPLAQUE);
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

  option_t *opt = menu_current->options + menu_current->selection;

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

static void DrawOption(s32 x, s32 y, const option_t *opt, const qboolean selected) {
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
  Scr_DrawPic(32, 16, C_WHITE, pic_quake);

  s32 basex = 32 + 16 + pic_quake->size.u;
  s32 basey = 16 + 8;
  Scr_DrawTextOffset(128, basex, basey, C_WHITE, menu_current->title);
  basey += 16;

  for (int i = 0; i < menu_current->numoptions; ++i) {
    const option_t *opt = menu_current->options + i;
    DrawOption(basex, basey, opt, i == menu_current->selection);
    basey += 8;
  }
}

qboolean Menu_IsOpen(void) {
  return menu_current != NULL;
}

static void Menu_StartNewGame(option_t *self) {
  menu_current = NULL;
  G_NewGame();
}

static void Menu_DbgStartGame(option_t *self) {
  char tmp[MAX_OSPATH];
  const char *ep = choices_episodes[dbg_episode];
  const char *map = (dbg_episode > 0 && dbg_episode < 5) ? choices_map[dbg_map] : "";
  snprintf(tmp, sizeof(tmp), "%s%s", ep, map);
  menu_current = NULL;
  gs.skill = dbg_skill;
  G_RequestMap(tmp);
}

static void Menu_DbgEpChanged(option_t *self) {
  // if a "special" episode is selected, lock the map selector to a single option
  if (dbg_episode == 0 || dbg_episode == 5) {
    options_select_map[1].choice.choices = &choices_episodes[dbg_episode];
    options_select_map[1].choice.numchoices = 1;
    dbg_map = 0;
    return;
  }

  // in case we were just on a "special" episode
  if (options_select_map[1].choice.choices != choices_map)
    options_select_map[1].choice.choices = choices_map;

  // E1 and E4 have 8 maps each, E2 and E3 have 7
  options_select_map[1].choice.numchoices = 7 + (dbg_episode == 1 || dbg_episode == 4);
  if (dbg_map >= options_select_map[1].choice.numchoices)
    dbg_map = options_select_map[1].choice.numchoices - 1;
}
