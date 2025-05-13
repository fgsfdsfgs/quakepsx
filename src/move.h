#pragma once

#include "common.h"
#include "entity.h"
#include "world.h"

#define MOVE_NORMAL 0
#define MOVE_NOMONSTERS 1
#define MOVE_MISSILE 2

#define G_MAXSPEED TO_FIX32(320)
#define G_ACCELERATE TO_FIX32(10)
#define G_FRICTION TO_FIX32(4)
#define G_STOPSPEED TO_FIX32(100)
#define G_GRAVITY TO_FIX32(800)

#define G_FORWARDSPEED TO_FIX32(200)
#define G_JUMPSPEED TO_FIX32(270)
#define G_YAWSPEED TO_DEG16(120)
#define G_PITCHSPEED TO_DEG16(130)

// 2 * G_FORWARDSPEED / G_MAXSPEED
#define G_DOUBLESPEEDFRAC TO_FIX32(3277) // ~0.8

#define G_FLOORNORMALZ 2867 // TO_FIX32(0.7)

#define G_STEPSIZE TO_FIX32(18)

typedef struct {
  x32vec3_t boxmins, boxmaxs;   // enclose the test object along entire move
  const x32vec3_t *mins, *maxs; // size of the moving object
  x32vec3_t mins2, maxs2;       // size when clipping against mosnters
  const x32vec3_t *start, *end;
  trace_t trace, tmptrace;
  int type;
  const edict_t *passedict;
} moveclip_t;

typedef struct {
  x32vec3_t wishvel;
  x16vec3_t wishdir;
  x32vec3_t *origin;
  x32vec3_t *velocity;
  x32 wishspeed;
  qboolean onground;
} moveplayer_t;

typedef struct {
  moveclip_t clip;
  moveplayer_t pm;
} movevars_t;

// this resides in the scratch and is used by G_Move and G_PlayerMove
extern movevars_t *const movevars;

edict_t *G_TestEntityPosition(const edict_t *ent);
const trace_t *G_Move(const x32vec3_t *start, const x32vec3_t *mins, const x32vec3_t *maxs, const x32vec3_t *end, const int type, const edict_t *passedict);

int G_FlyMove(edict_t *ent, x16 time, const trace_t **steptrace);
void G_WalkMove(edict_t *ent);
void G_PushMove(edict_t *pusher, x16 time);

qboolean G_DropToFloor(edict_t *ent);
qboolean G_CheckBottom(edict_t *ent);

void PM_PlayerMove(const x16 dt);

void G_Physics(void);
