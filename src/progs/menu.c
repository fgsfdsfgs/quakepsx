#include "prcommon.h"
#include "menu.h"
#include "cd.h"

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

static void Menu_StartNewGame(menuoption_t *self);
static void Menu_DbgStartGame(menuoption_t *self);
static void Menu_DbgEpChanged(menuoption_t *self);
static void Menu_CdVolChanged(menuoption_t *self) { CD_SetAudioVolume(cd_volume); }
static void Menu_SndVolChanged(menuoption_t *self) { Snd_SetVolume(snd_volume); }

static menuoption_t options_options[] = {
  { OPT_SLIDER, "Sound Volume", &snd_volume, Menu_SndVolChanged, { .slider = { 0, 128, 16        } } },
  { OPT_SLIDER, "Music Volume", &cd_volume,  Menu_CdVolChanged,  { .slider = { 0, 128, 16        } } },
  { OPT_CHOICE, "Crosshair",    &sbar_xhair, NULL,               { .choice = { choices_off_on, 2 } } },
};

static menu_t menu_options = {
  "Options",
  options_options, 3,
  0, NULL
};

static menuoption_t options_select_map[] = {
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

static menuoption_t options_main[] = {
  { OPT_BUTTON, "New game",   NULL, Menu_StartNewGame, {                                } },
  { OPT_BUTTON, "Select map", NULL, NULL,              { .button = { &menu_select_map } } },
  { OPT_BUTTON, "Options",    NULL, NULL,              { .button = { &menu_options    } } },
};

menu_t menu_main = {
  "Main",
  options_main, 3,
  0, NULL
};

static void Menu_StartNewGame(menuoption_t *self) {
  Menu_Close();
  G_NewGame();
}

static void Menu_DbgStartGame(menuoption_t *self) {
  char tmp[MAX_OSPATH];
  const char *ep = choices_episodes[dbg_episode];
  const char *map = (dbg_episode > 0 && dbg_episode < 5) ? choices_map[dbg_map] : "";
  snprintf(tmp, sizeof(tmp), "%s%s", ep, map);
  gs.skill = dbg_skill;
  Menu_Close();
  G_RequestMap(tmp);
}

static void Menu_DbgEpChanged(menuoption_t *self) {
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
