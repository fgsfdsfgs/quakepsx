#pragma once

#include "common.h"
#include "model.h"

#define MAX_ENT_LEAFS 8

// edict->solid values
#define SOLID_NOT      0  // no interaction with other objects
#define SOLID_TRIGGER  1  // touch on edge, but not blocking
#define SOLID_BBOX     2  // touch on edge, block
#define SOLID_SLIDEBOX 3  // touch on edge, but not an onground
#define SOLID_BSP      4  // bsp clip, touch on edge, block

// edict->movetype values
#define MOVETYPE_NONE        0  // never moves
#define MOVETYPE_ANGLENOCLIP 1
#define MOVETYPE_ANGLECLIP   2
#define MOVETYPE_WALK        3  // gravity
#define MOVETYPE_STEP        4  // gravity, special edge handling
#define MOVETYPE_FLY         5
#define MOVETYPE_TOSS        6  // gravity
#define MOVETYPE_PUSH        7  // no clip to world, push and crush
#define MOVETYPE_NOCLIP      8
#define MOVETYPE_FLYMISSILE  9  // extra size to monsters
#define MOVETYPE_BOUNCE      10

// edict->flags
#define FL_FLY           (1 <<  0)
#define FL_SWIM          (1 <<  1)
#define FL_CONVEYOR      (1 <<  2)
#define FL_CLIENT        (1 <<  3)
#define FL_INWATER       (1 <<  4)
#define FL_MONSTER       (1 <<  5)
#define FL_GODMODE       (1 <<  6)
#define FL_NOTARGET      (1 <<  7)
#define FL_ITEM          (1 <<  8)
#define FL_ONGROUND      (1 <<  9)
#define FL_PARTIALGROUND (1 << 10) // not all corners are valid
#define FL_WATERJUMP     (1 << 11) // player jumping out of water
#define FL_VISIBLE       (1 << 12) // was rendered last frame
#define FL_DEAD          (1 << 13)
#define FL_TAKEDAMAGE    (1 << 14)
#define FL_AUTOAIM       (1 << 15)

// entity effects
#define EF_BRIGHTFIELD 1
#define EF_MUZZLEFLASH 2
#define EF_BRIGHTLIGHT 4
#define EF_DIMLIGHT    8

// spawnflags
#define SPAWNFLAG_NOT_EASY       256
#define SPAWNFLAG_NOT_MEDIUM     512
#define SPAWNFLAG_NOT_HARD       1024
#define SPAWNFLAG_NOT_DEATHMATCH 2048
#define SPAWNFLAG_SKILL_MASK     (SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD)

#define EDICT_NUM(e) ((e) - gs.edicts)

typedef struct edict_s edict_t;
typedef struct player_state_s player_state_t;
typedef struct monster_fields_s monster_fields_t;

typedef void (*think_fn_t)(edict_t *self);
typedef void (*interact_fn_t)(edict_t *self, edict_t *other);
typedef void (*damage_fn_t)(edict_t *self, edict_t *attacker, s16 damage);
typedef qboolean (*check_fn_t)(edict_t *self);

typedef struct entvars_s {
  u8 classname;
  u8 solid;
  u8 movetype;
  u8 waterlevel;
  s8 watertype;
  u8 noise;
  u8 light;
  u8 effects;
  u8 cnt;
  u16 target;
  u16 targetname;
  u16 flags;
  u16 spawnflags;
  s16 modelnum;
  s16 frame;
  s16 health;
  s16 max_health;
  x32 nextthink;
  x32 ltime;
  x32 viewheight;
  void *model; // either model_t or aliashdr_t
  think_fn_t think;
  interact_fn_t touch;
  interact_fn_t blocked;
  interact_fn_t use;
  think_fn_t th_die;
  edict_t *owner;
  edict_t *groundentity;
  edict_t *chain;
  x32vec3_t absmin, mins;
  x32vec3_t absmax, maxs;
  x32vec3_t size;
  x32vec3_t origin;
  x32vec3_t oldorigin;
  x32vec3_t velocity;
  x16vec3_t avelocity;
  x16vec3_t angles;
  // class-specific extra data
  union {
    void *extra_ptr;
    u32 extra_field;
    player_state_t *player;
    monster_fields_t *monster;
    struct {
      u32 type;
      u16 ammotype;
      u16 count;
    } extra_item;
  };
} entvars_t;

struct edict_s {
  qboolean free;
  s8 num_leafs;
  s16 leafnums[MAX_ENT_LEAFS];
  link_t area;
  x32 freetime;
  entvars_t v;
};

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, edict_t, area)

edict_t *ED_Alloc(void);
void ED_Free(edict_t *ed);
