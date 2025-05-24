#include "prcommon.h"
#include "menu.h"
#include "cd.h"
#include "input.h"

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
static void Menu_ActivateCheat(menuoption_t *self);
static void Menu_CdVolChanged(menuoption_t *self) { CD_SetAudioVolume(cd_volume); }
static void Menu_SndVolChanged(menuoption_t *self) { Snd_SetVolume(snd_volume); }

static menuoption_t options_options[] = {
  { OPT_LABEL,  "Audio",        NULL,                  NULL,               {                                 } },
  { OPT_SLIDER, "Sound Volume", &snd_volume,           Menu_SndVolChanged, { .slider = { 0, 128, 16        } } },
  { OPT_SLIDER, "Music Volume", &cd_volume,            Menu_CdVolChanged,  { .slider = { 0, 128, 16        } } },
  { OPT_LABEL,  "Input",        NULL,                  NULL,               {                                 } },
  { OPT_SLIDER, "Mouse Speed",  &in.mouse_sens,        NULL,               { .slider = { 0, 64, 8          } } },
  { OPT_SLIDER, "LS Speed X",   &in.stick_sens[0].x,   NULL,               { .slider = { 0, 8, 1           } } },
  { OPT_SLIDER, "LS Speed Y",   &in.stick_sens[0].y,   NULL,               { .slider = { 0, 8, 1           } } },
  { OPT_SLIDER, "LS Deadzone",  &in.stick_deadzone[0], NULL,               { .slider = { 0, 64, 8          } } },
  { OPT_SLIDER, "RS Speed X",   &in.stick_sens[1].x,   NULL,               { .slider = { 0, 32, 4          } } },
  { OPT_SLIDER, "RS Speed Y",   &in.stick_sens[1].y,   NULL,               { .slider = { 0, 32, 4          } } },
  { OPT_SLIDER, "RS Deadzone",  &in.stick_deadzone[1], NULL,               { .slider = { 0, 64, 8          } } },
  { OPT_LABEL,  "Game",         NULL,                  NULL,               {                                 } },
  { OPT_CHOICE, "Crosshair",    &sbar_xhair,           NULL,               { .choice = { choices_off_on, 2 } } },
};

static menu_t menu_options = {
  "Options",
  options_options, ARRAYCOUNT(options_options),
  0, NULL
};

static menuoption_t options_cheats[] = {
  { OPT_BUTTON, "Noclip",      NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "God mode",    NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "All weapons", NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Full health", NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Full armor",  NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "All keys",    NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Give quad",   NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Give pent",   NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Give ring",   NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Give suit",   NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Notarget ",   NULL, Menu_ActivateCheat, { } },
  { OPT_BUTTON, "Suicide",     NULL, Menu_ActivateCheat, { } },
};

static menu_t menu_cheats = {
  "Cheats",
  options_cheats, 12,
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
  { OPT_BUTTON, "Cheats",     NULL, NULL,              { .button = { &menu_cheats     } } },
  { OPT_BUTTON, "Options",    NULL, NULL,              { .button = { &menu_options    } } },
};

menu_t menu_main = {
  "Main",
  options_main, 4,
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

static void Menu_ActivateCheat(menuoption_t *self) {
  edict_t *ent = gs.player->ent;
  player_state_t *plr = gs.player;

  Menu_Close();

  switch (self - options_cheats) {
  case 0: /* noclip */
    if (ent->v.movetype == MOVETYPE_NOCLIP)
      ent->v.movetype = MOVETYPE_WALK;
    else if (ent->v.movetype == MOVETYPE_WALK)
      ent->v.movetype = MOVETYPE_NOCLIP;
    break;

  case 1: /* god mode */
    ent->v.flags ^= FL_GODMODE;
    break;

  case 2: /* all weapons*/
    plr->stats.items |= IT_SUPER_SHOTGUN | IT_NAILGUN | IT_SUPER_NAILGUN | IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_LIGHTNING;
    plr->stats.ammo[AMMO_SHELLS] = 100;
    plr->stats.ammo[AMMO_NAILS] = 200;
    plr->stats.ammo[AMMO_ROCKETS] = 100;
    plr->stats.ammo[AMMO_CELLS] = 100;
    break;

  case 3: /* full health */
    ent->v.health = 100;
    break;

  case 4: /* full armor */
    plr->stats.armor = 150;
    plr->stats.armortype = ARMOR_TYPE3;
    plr->stats.items |= IT_ARMOR3;
    break;

  case 5: /* all keys */
    plr->stats.items |= IT_KEY1 | IT_KEY2;
    break;

  case 6: /* give quad */
    plr->stats.items |= IT_QUAD;
    plr->power_time[POWER_QUAD] = gs.time + FTOX(30.0);
    plr->power_warn[POWER_QUAD] = 1;
    break;

  case 7: /* give pent */
    plr->stats.items |= IT_INVULNERABILITY;
    plr->power_time[POWER_INVULN] = gs.time + FTOX(30.0);
    plr->power_warn[POWER_INVULN] = 1;
    break;

  case 8: /* give ring */
    plr->stats.items |= IT_INVISIBILITY;
    plr->power_time[POWER_INVIS] = gs.time + FTOX(30.0);
    plr->power_warn[POWER_INVIS] = 1;
    break;

  case 9: /* give suit */
    plr->stats.items |= IT_SUIT;
    plr->power_time[POWER_SUIT] = gs.time + FTOX(30.0);
    plr->power_warn[POWER_SUIT] = 1;
    break;

  case 10: /* notarget */
    if (ent->v.flags & FL_NOTARGET)
      ent->v.flags &= ~FL_NOTARGET;
    else
      ent->v.flags |= FL_NOTARGET;
    break;

  case 11: /* suicide */
    ent->v.flags &= ~FL_GODMODE;
    plr->stats.items &= ~IT_INVULNERABILITY;
    plr->power_time[POWER_INVULN] = 0;
    utl_damage(ent, ent, ent, 6666);
    break;

  default:
    break;
  }
}