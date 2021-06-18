#pragma once

#include "common.h"
#include "entity.h"

typedef struct player_state_s {
  x16vec3_t viewangles;
  x16vec3_t punchangle;
  x32vec3_t move;
  edict_t *ent;
} player_state_t;

typedef struct game_state_s {
  x32 time;
  qboolean paused;
  model_t *worldmodel;
  player_state_t player[MAX_PLAYERS];
} game_state_t;

extern game_state_t gs;
