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
#define FL_FLY           1
#define FL_SWIM          2
#define FL_CONVEYOR      4
#define FL_CLIENT        8
#define FL_INWATER       16
#define FL_MONSTER       32
#define FL_GODMODE       64
#define FL_NOTARGET      128
#define FL_ITEM          256
#define FL_ONGROUND      512
#define FL_PARTIALGROUND 1024  // not all corners are valid
#define FL_WATERJUMP     2048  // player jumping out of water
#define FL_JUMPRELEASED  4096  // for jump debouncing

typedef struct edict_s edict_t;

typedef void (*think_fn_t)(edict_t *self);
typedef void (*interact_fn_t)(edict_t *self, edict_t *other);

typedef struct entvars_s {
  u8 classname;
  u8 solid;
  u8 movetype;
  u8 waterlevel;
  u8 watertype;
  u16 flags;
  s16 modelnum;
  s16 frame;
  x32 nextthink;
  x32 ltime;
  x32 viewheight;
  void *model; // either model_t or aliashdr_t
  think_fn_t think;
  interact_fn_t touch;
  interact_fn_t blocked;
  interact_fn_t use;
  edict_t *owner;
  edict_t *groundentity;
  x32vec3_t absmin, mins;
  x32vec3_t absmax, maxs;
  x32vec3_t size;
  x32vec3_t origin;
  x32vec3_t oldorigin;
  x32vec3_t velocity;
  x16vec3_t avelocity;
  x16vec3_t angles;
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

edict_t *ED_Alloc(u8 classname);
void ED_Free(edict_t *ed);
