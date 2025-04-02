#pragma once

// progs stuff that is directly used by the engine

#include "common.h"
#include "progs/entclasses.h"
#include "progs/picids.h"

struct player_state_s;

// status bar
void Sbar_Init(void);
void Sbar_Draw(const struct player_state_s *p);

// player
void Player_PreThink(edict_t *ent);
void Player_PostThink(edict_t *ent);
