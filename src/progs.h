#pragma once

// progs stuff that is directly used by the engine

#include "common.h"
#include "game.h"
#include "menu.h"
#include "progs/entclasses.h"
#include "progs/picids.h"

// progs' main menu
extern menu_t menu_main;

// global
void Progs_NewMap(void);

// status bar
void Sbar_Init(void);
void Sbar_Draw(const player_state_t *p);

// player
void Player_PreThink(edict_t *ent);
void Player_PostThink(edict_t *ent);
