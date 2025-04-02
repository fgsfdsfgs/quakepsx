#pragma once

#include "../common.h"
#include "../game.h"
#include "../entity.h"
#include "../sound.h"
#include "../screen.h"
#include "../system.h"
#include "entclasses.h"
#include "mdlids.h"
#include "picids.h"
#include "sfxids.h"

#define PR_FRAMETIME 410 // ~0.1 sec

// player stuff
#define PLAYER_HEALTH 100

// items
#define IT_SHOTGUN          1u
#define IT_SUPER_SHOTGUN    2u
#define IT_NAILGUN          4u
#define IT_SUPER_NAILGUN    8u
#define IT_GRENADE_LAUNCHER 16u
#define IT_ROCKET_LAUNCHER  32u
#define IT_LIGHTNING        64u
#define IT_SUPER_LIGHTNING  128u
#define IT_SHELLS           256u
#define IT_NAILS            512u
#define IT_ROCKETS          1024u
#define IT_CELLS            2048u
#define IT_AXE              4096u
#define IT_ARMOR1           8192u
#define IT_ARMOR2           16384u
#define IT_ARMOR3           32768u
#define IT_SUPERHEALTH      65536u
#define IT_KEY1             131072u
#define IT_KEY2             262144u
#define IT_INVISIBILITY     524288u
#define IT_INVULNERABILITY  1048576u
#define IT_SUIT             2097152u
#define IT_QUAD             4194304u
#define IT_SIGIL1           (1u << 28)
#define IT_SIGIL2           (1u << 29)
#define IT_SIGIL3           (1u << 30)
#define IT_SIGIL4           (1u << 31)

// ammo
#define AMMO_SHELLS 0
#define AMMO_NAILS 1
#define AMMO_ROCKETS 2
#define AMMO_CELLS 3

// common thinkers
void null_think(edict_t *self);
void cycler_think(edict_t *self, const s16 idle_start, const s16 idle_end);

// status bar
void Sbar_IndicateDamage(void);
