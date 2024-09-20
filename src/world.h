#pragma once

#include "common.h"
#include "entity.h"

#define AREA_DEPTH 4
#define AREA_NODES 32

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

typedef struct areanode_s
{
  s32 axis; // -1 = leaf node
  x32 dist;
  struct areanode_s *children[2];
  link_t trigger_edicts;
  link_t solid_edicts;
} areanode_t;

extern areanode_t g_areanodes[AREA_NODES];
extern int g_numareanodes;

void G_InitBoxHull(void);
hull_t *G_HullForEntity(edict_t *ent, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *offset);

int G_HullPointContents(hull_t *hull, int num, x32vec3_t *p);
int G_PointContents(x32vec3_t *p);
int G_TruePointContents(x32vec3_t *p);

qboolean G_RecursiveHullCheck(hull_t *hull, int num, x32 p1f, x32 p2f, x32vec3_t *p1, x32vec3_t *p2, trace_t *trace);

void G_ClearWorld();
void G_UnlinkEdict(edict_t *ent);
void G_LinkEdict(edict_t *ent, qboolean touch_triggers);

void G_MoveBounds(const x32vec3_t *start, const x32vec3_t *mins, const x32vec3_t *maxs, const x32vec3_t *end, x32vec3_t *boxmins, x32vec3_t *boxmaxs);
