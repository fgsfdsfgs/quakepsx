#include "common.h"
#include "entity.h"
#include "model.h"
#include "game.h"
#include "system.h"
#include "world.h"

static hull_t box_hull;
static xbspclipnode_t box_clipnodes[6];
static mplane_t box_planes[6];

void G_InitBoxHull(void)
{
  int i;
  int side;

  box_hull.clipnodes = box_clipnodes;
  box_hull.planes = box_planes;
  box_hull.firstclipnode = 0;
  box_hull.lastclipnode = 5;

  for (i = 0; i < 6; i++)
  {
    box_clipnodes[i].planenum = i;

    side = i&1;

    box_clipnodes[i].children[side] = CONTENTS_EMPTY;
    if (i != 5)
      box_clipnodes[i].children[side^1] = i + 1;
    else
      box_clipnodes[i].children[side^1] = CONTENTS_SOLID;

    box_planes[i].type = i >> 1;
    box_planes[i].normal.d[i >> 1] = 1;
  }
}

int G_HullPointContents(hull_t *hull, int num, x32vec3_t *p)
{
  x32 d;
  xbspclipnode_t *node;
  mplane_t *plane;

  while (num >= 0)
  {
    if (num < hull->firstclipnode || num > hull->lastclipnode)
      Sys_Error("G_HullPointContents: node %d out of range (%d, %d)", num, (int)hull->firstclipnode, (int)hull->lastclipnode);

    node = hull->clipnodes + num;
    plane = hull->planes + node->planenum;

    if (plane->type < 3)
      d = p->d[plane->type] - plane->dist;
    else
      d = XVecDotSL(&plane->normal, p) - plane->dist;
    if (d < 0)
      num = node->children[1];
    else
      num = node->children[0];
  }

  return num;
}

int G_PointContents(x32vec3_t *p)
{
  int  cont;

  cont = G_HullPointContents(&gs.worldmodel->hulls[0], 0, p);
  if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
    cont = CONTENTS_WATER;
  return cont;
}

int G_TruePointContents(x32vec3_t *p)
{
  return G_HullPointContents(&gs.worldmodel->hulls[0], 0, p);
}

#define DIST_EPSILON 128

qboolean G_RecursiveHullCheck(hull_t *hull, int num, x32 p1f, x32 p2f, x32vec3_t *p1, x32vec3_t *p2, trace_t *trace)
{
  xbspclipnode_t *node;
  mplane_t *plane;
  x32 t1, t2;
  x32 frac;
  int i;
  x32vec3_t mid;
  int side;
  x32 midf;

  // check for empty
  if (num < 0)
  {
    if (num != CONTENTS_SOLID)
    {
      trace->allsolid = false;
      if (num == CONTENTS_EMPTY)
        trace->inopen = true;
      else
        trace->inwater = true;
    }
    else
      trace->startsolid = true;
    return true; // empty
  }

  if (num < hull->firstclipnode || num > hull->lastclipnode)
    Sys_Error("G_RecursiveHullCheck: bad node number");

  //
  // find the point distances
  //
  node = hull->clipnodes + num;
  plane = hull->planes + node->planenum;

  if (plane->type < 3)
  {
    t1 = p1->d[plane->type] - plane->dist;
    t2 = p2->d[plane->type] - plane->dist;
  }
  else
  {
    t1 = XVecDotSL(&plane->normal, p1) - plane->dist;
    t2 = XVecDotSL(&plane->normal, p2) - plane->dist;
  }

  if (t1 >= 0 && t2 >= 0)
    return G_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2, trace);
  if (t1 < 0 && t2 < 0)
    return G_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2, trace);

  // put the crosspoint DIST_EPSILON pixels on the near side
  if (t1 < 0)
    frac = xdiv32(t1 + DIST_EPSILON, t1 - t2);
  else
    frac = xdiv32(t1 - DIST_EPSILON, t1 - t2);
  if (frac < 0)
    frac = 0;
  if (frac > ONE)
    frac = ONE;

  midf = p1f + xmul32(frac, p2f - p1f);
  for (i = 0; i < 3; i++)
    mid.d[i] = p1->d[i] + xmul32(frac, p2->d[i] - p1->d[i]);

  side = (t1 < 0);

  // move up to the node
  if (!G_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, &mid, trace))
    return false;

  // go past the node
  if (G_HullPointContents(hull, node->children[side^1], &mid) != CONTENTS_SOLID)
    return G_RecursiveHullCheck(hull, node->children[side^1], midf, p2f, &mid, p2, trace);

  if (trace->allsolid)
    return false; // never got out of the solid area

  // the other side of the node is solid, this is the impact point
  if (!side)
  {
    trace->plane.normal = plane->normal;
    trace->plane.dist = plane->dist;
  }
  else
  {
    XVecSub(&x32vec3_origin, &plane->normal, &trace->plane.normal);
    trace->plane.dist = -plane->dist;
  }

  trace->fraction = midf;
  trace->endpos = mid;

  return false;
}

hull_t *G_HullForBox(x32vec3_t *mins, x32vec3_t *maxs)
{
 box_planes[0].dist = maxs->d[0];
 box_planes[1].dist = mins->d[0];
 box_planes[2].dist = maxs->d[1];
 box_planes[3].dist = mins->d[1];
 box_planes[4].dist = maxs->d[2];
 box_planes[5].dist = mins->d[2];
 return &box_hull;
}

hull_t *G_HullForEntity(edict_t *ent, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *offset)
{
  model_t *model;
  x32vec3_t size;
  x32vec3_t hullmins, hullmaxs;
  hull_t *hull;

  // decide which clipping hull to use, based on the size
  if (ent->v.solid == SOLID_BSP)
  {
    // explicit hulls in the BSP model
    model = ent->v.model;

    XVecSub(maxs, mins, &size);
    if (size.d[0] < TO_FIX32(3))
      hull = &model->hulls[0];
    else if (size.d[0] <= TO_FIX32(32))
      hull = &model->hulls[1];
    else
      hull = &model->hulls[2];

    // calculate an offset value to center the origin
    XVecSub(&hull->mins, mins, offset);
    XVecAdd(offset, &ent->v.origin, offset);
  }
  else
  {
    // create a temp hull from bounding box sizes
    XVecSub(&ent->v.mins, maxs, &hullmins);
    XVecSub(&ent->v.maxs, mins, &hullmaxs);
    hull = G_HullForBox(&hullmins, &hullmaxs);
    *offset = ent->v.origin;
  }

  return hull;
}
