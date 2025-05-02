#pragma once

#include "common.h"
#include "model.h"
#include "entity.h"

#define MAX_AMMO_TYPES 4

enum player_flags_e {
  PFL_JUMPED = (1 << 0),
  PFL_INWATER = (1 << 1),
};

typedef struct stats_s {
  s16 armor;
  s16 armortype;
  s16 weaponnum;
  s16 ammonum;
  u32 items;
  s16 ammo[MAX_AMMO_TYPES];
} stats_t;

typedef struct player_state_s {
  stats_t stats;
  x16vec3_t viewangles;
  x16vec3_t punchangle;
  x16vec3_t anglemove;
  s16vec3_t vmodelofs;
  x32vec3_t move;
  x32vec3_t viewofs;
  edict_t *ent;
  amodel_t *vmodel;
  x32 movespeed;
  x32 fallspeed;
  u32 buttons;
  u16 flags;
  s16 vmodelframe;
} player_state_t;

typedef struct game_state_s {
  x32 time;
  x16 frametime;
  s16 num_edicts;
  s16 max_edict;
  s16 force_retouch;
  qboolean paused;
  bmodel_t *worldmodel;
  bmodel_t **bmodels;
  amodel_t *amodels;
  edict_t *edicts;
  player_state_t player[MAX_PLAYERS];
} game_state_t;

extern game_state_t gs;

void G_RequestMap(const char *mapname);
qboolean G_CheckNextMap(void);
void G_StartMap(const char *path);
void G_Update(const x16 dt);

amodel_t *G_FindAliasModel(const s16 modelid);
void G_SetModel(edict_t *ent, s16 modelnum);
void G_SetSize(edict_t *ent, const x32vec3_t *mins, const x32vec3_t *maxs);
