#pragma once

#include "common.h"
#include "model.h"
#include "entity.h"

typedef struct player_state_s {
  x16vec3_t viewangles;
  x16vec3_t punchangle;
  x16vec3_t anglemove;
  x32vec3_t move;
  x32vec3_t viewofs;
  edict_t *ent;
  x32 movespeed;
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

void G_StartMap(const char *path);
void G_Update(const x16 dt);

void G_SetModel(edict_t *ent, s16 modelnum);
void G_SetSize(edict_t *ent, const x32vec3_t *mins, const x32vec3_t *maxs);
