#pragma once

#include "common.h"
#include "entity.h"

#define PLAYER_WALK_SPEED TO_FIX32(400)
#define PLAYER_JUMP_SPEED TO_FIX32(200)
#define PLAYER_LOOK_SPEED TO_DEG16(60)

#define PLAYER_ACCELERATION TO_FIX32(600)
#define PLAYER_FRICTION     TO_FIX32(400)

typedef struct player_state_s {
  x16vec3_t viewangles;
  x16vec3_t punchangle;
  x16vec3_t anglemove;
  x32vec3_t move;
  x32vec3_t viewofs;
  edict_t *ent;
} player_state_t;

typedef struct game_state_s {
  x32 time;
  x16 frametime;
  s16 num_edicts;
  qboolean paused;
  model_t *worldmodel;
  model_t **models;
  edict_t *edicts;
  player_state_t player[MAX_PLAYERS];
} game_state_t;

extern game_state_t gs;

void G_StartMap(const char *path);
void G_PlayerMove(const x16 dt);
