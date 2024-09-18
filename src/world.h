#pragma once

#include "common.h"
#include "entity.h"

typedef struct {
  x16vec3_t normal;
  x32 dist;
} plane_t;

typedef struct {
  qboolean allsolid;   // if true, plane is not valid
  qboolean startsolid; // if true, the initial point was in a solid area
  qboolean inopen;
  qboolean inwater;
  x16 fraction;        // time completed, 1.0 = didn't hit anything
  x32vec3_t endpos;    // final position
  plane_t plane;       // surface normal at impact
  edict_t *ent;        // entity the surface is on
} trace_t;

void G_InitBoxHull(void);
hull_t *G_HullForEntity(edict_t *ent, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *offset);

int G_HullPointContents(hull_t *hull, int num, x32vec3_t *p);
int G_PointContents(x32vec3_t *p);
int G_TruePointContents(x32vec3_t *p);

qboolean G_RecursiveHullCheck(hull_t *hull, int num, x32 p1f, x32 p2f, x32vec3_t *p1, x32vec3_t *p2, trace_t *trace);

void G_UnlinkEdict(edict_t *ent);
void G_LinkEdict(edict_t *ent, qboolean touch_triggers);
