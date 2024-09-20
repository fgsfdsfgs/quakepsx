#include <string.h>

#include "common.h"
#include "world.h"
#include "entity.h"
#include "game.h"
#include "move.h"

movevars_t *const movevars = PSX_SCRATCH;

#define MAX_CLIP_PLANES 4
#define STOP_EPSILON 256
#define STEPSIZE TO_FIX32(18)
#define MAX_PUSHED 32

static int ClipVelocity(x32vec3_t *in, x16vec3_t *normal, x32vec3_t *out, const x16 overbounce)
{
  x32 backoff;
  x32 change;
  int i, blocked;

  blocked = 0;
  if (normal->z > 0)
    blocked |= 1; // floor
  else if (!normal->z)
    blocked |= 2; // step

  backoff = xmul32(overbounce, XVecDotSL(normal, in));

  for (i = 0; i < 3; i++)
  {
    change = xmul32(normal->d[i], backoff);
    out->d[i] = in->d[i] - change;
    if (out->d[i] > -STOP_EPSILON && out->d[i] < STOP_EPSILON)
      out->d[i] = 0;
  }

  return blocked;
}

static void G_ClipMoveToEntity(edict_t *ent, x32vec3_t *start, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *end, trace_t *trace)
{
  x32vec3_t offset;
  x32vec3_t start_l, end_l;
  hull_t *hull;

  // fill in a default trace
  trace->fraction = ONE;
  trace->allsolid = true;
  trace->startsolid = false;
  trace->endpos = *end;

  // get the clipping hull
  hull = G_HullForEntity(ent, mins, maxs, &offset);

  XVecSub(start, &offset, &start_l);
  XVecSub(end, &offset, &end_l);

  // trace a line through the apropriate clipping hull
  G_RecursiveHullCheck(hull, hull->firstclipnode, 0, ONE, &start_l, &end_l, trace);

  // fix trace up by the offset
  if (trace->fraction != ONE)
    XVecAdd(&trace->endpos, &offset, &trace->endpos);

  // did we clip the move?
  if (trace->fraction < ONE || trace->startsolid)
    trace->ent = ent;
}

void G_ClipToLinks(areanode_t *node, moveclip_t *clip)
{
  link_t *l, *next;
  edict_t *touch;
  trace_t *trace = &clip->tmptrace;

  // touch linked edicts
  for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
  {
    next = l->next;
    touch = EDICT_FROM_AREA(l);
    if (touch->v.solid == SOLID_NOT)
      continue;
    if (touch == clip->passedict)
      continue;

    if (clip->type == MOVE_NOMONSTERS && touch->v.solid != SOLID_BSP)
      continue;

    if (clip->boxmins.d[0] > touch->v.absmax.d[0]
    || clip->boxmins.d[1] > touch->v.absmax.d[1]
    || clip->boxmins.d[2] > touch->v.absmax.d[2]
    || clip->boxmaxs.d[0] < touch->v.absmin.d[0]
    || clip->boxmaxs.d[1] < touch->v.absmin.d[1]
    || clip->boxmaxs.d[2] < touch->v.absmin.d[2] )
      continue;

    if (clip->passedict && clip->passedict->v.size.x && !touch->v.size.x)
      continue;	// points never interact

    // might intersect, so do an exact clip
    if (clip->trace.allsolid)
      return;
    if (clip->passedict)
    {
      if (touch->v.owner == clip->passedict)
        continue;	// don't clip against own missiles
      if (clip->passedict->v.owner == touch)
        continue;	// don't clip against owner
    }

    if (touch->v.flags & FL_MONSTER)
      G_ClipMoveToEntity(touch, clip->start, &clip->mins2, &clip->maxs2, clip->end, &clip->tmptrace);
    else
      G_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end, &clip->tmptrace);

    if (trace->allsolid || trace->startsolid || trace->fraction < clip->trace.fraction)
    {
      trace->ent = touch;
      if (clip->trace.startsolid)
      {
        clip->trace = *trace;
        clip->trace.startsolid = true;
      }
      else
        clip->trace = *trace;
    }
    else if (trace->startsolid)
      clip->trace.startsolid = true;
  }

  // recurse down both sides
  if (node->axis == -1)
    return;

  if (clip->boxmaxs.d[node->axis] > node->dist)
    G_ClipToLinks(node->children[0], clip);
  if (clip->boxmins.d[node->axis] < node->dist)
    G_ClipToLinks(node->children[1], clip);
}

const trace_t *G_Move(x32vec3_t *start, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *end, int type, edict_t *passedict)
{
  moveclip_t *clip = &movevars->clip;
  int i;

  memset(clip, 0, sizeof(moveclip_t));

  // clip to world
  G_ClipMoveToEntity(gs.edicts, start, mins, maxs, end, &clip->trace);

  clip->start = start;
  clip->end = end;
  clip->mins = mins;
  clip->maxs = maxs;
  clip->type = type;
  clip->passedict = passedict;

  if (type == MOVE_MISSILE)
  {
    for (i = 0; i < 3; i++)
    {
      clip->mins2.d[i] = -TO_FIX32(15);
      clip->maxs2.d[i] = TO_FIX32(15);
    }
  }
  else
  {
    clip->mins2 = *mins;
    clip->maxs2 = *maxs;
  }

  // create the bounding box of the entire move
  G_MoveBounds(start, &clip->mins2, &clip->maxs2, end, &clip->boxmins, &clip->boxmaxs);

  // clip to entities
  G_ClipToLinks(g_areanodes, clip);

  return &clip->trace;
}

int G_FlyMove(edict_t *ent, x16 time, const trace_t **steptrace)
{
  int bumpcount;
  x16vec3_t dir;
  x32 d;
  int numplanes;
  x16vec3_t planes[MAX_CLIP_PLANES];
  x16vec3_t original_signs;
  x32vec3_t original_velocity, new_velocity;
  int i, j;
  const trace_t *trace;
  x32vec3_t end;
  x16 time_left;
  int blocked;

  blocked = 0;
  numplanes = 0;
  original_velocity = ent->v.velocity;
  XVecSignLS(&ent->v.velocity, &original_signs);

  time_left = time;

  for (bumpcount = 0; bumpcount < 3; bumpcount++)
  {
    if (!ent->v.velocity.x && !ent->v.velocity.y && !ent->v.velocity.z)
      break;

    XVecScaleLS(&ent->v.velocity, time_left, &end);
    XVecAdd(&ent->v.origin, &end, &end);

    trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, false, ent);

    if (trace->allsolid)
    {
      // entity is trapped in another solid
      ent->v.velocity = x32vec3_origin;
      return 3;
    }

    if (trace->fraction > 0)
    {
      // actually covered some distance
      ent->v.origin = trace->endpos;
      original_velocity = ent->v.velocity;
      numplanes = 0;
    }

    if (trace->fraction == ONE)
        break; // moved the entire distance

    if (trace->plane.normal.z > 2867) // TO_FIX32(0.7)
    {
      blocked |= 1;  // floor
      if (trace->ent->v.solid == SOLID_BSP)
      {
        ent->v.flags |= FL_ONGROUND;
        // ent->v.groundentity = EDICT_TO_PROG(trace.ent);
      }
    }
    if (!trace->plane.normal.z)
    {
      blocked |= 2; // step
      if (steptrace)
        *steptrace = trace; // save for player extrafriction
    }

    //
    // run the impact function
    //
    // SV_Impact (ent, trace.ent);
    if (ent->free)
      break; // removed by the impact function

    time_left -= XMUL16(time_left, trace->fraction);

    planes[numplanes] = trace->plane.normal;
    numplanes++;

    //
    // modify original_velocity so it parallels all of the clip planes
    //
    for (i = 0; i < numplanes; i++)
    {
      ClipVelocity(&original_velocity, &planes[i], &new_velocity, ONE);
      for (j = 0; j < numplanes; j++)
      {
        if (j != i)
        {
          if (XVecDotSL(&planes[j], &new_velocity) < 0)
            break; // not ok
        }
      }
      if (j == numplanes)
        break;
    }
    
    if (i != numplanes)
    {
      // go along this plane
      ent->v.velocity = new_velocity;
    }
    else
    {
      // go along the crease
      if (numplanes != 2)
      {
        ent->v.velocity = x32vec3_origin;
        return 7;
      }
      XVecCrossSS(&planes[0], &planes[1], &dir);
      d = XVecDotSL(&dir, &ent->v.velocity);
      XVecScaleSL(&dir, d, &ent->v.velocity);
    }

    //
    // if original velocity is against the original velocity, stop dead
    // to avoid tiny occilations in sloping corners
    //
    if (XVecDotSL(&original_signs, &ent->v.velocity) <= 0)
    {
      ent->v.velocity = x32vec3_origin;
      return blocked;
    }
  }

  return blocked;
}

const trace_t *G_PushEntity(edict_t *ent, x32vec3_t *push)
{
  const trace_t *trace;
  x32vec3_t end;

  XVecAdd(&ent->v.origin, push, &end);

  trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, 0, ent);

  ent->v.origin = trace->endpos;
  G_LinkEdict(ent, true);

  return trace;
}

int G_TryUnstick(edict_t *ent, x32vec3_t *oldvel)
{
	int i;
	x32vec3_t oldorg = ent->v.origin;
	x32vec3_t dir = { 0 };
	int clip;
	const trace_t *steptrace;

  for (i = 0; i < 8; i++)
  {
    // try pushing a little in an axial direction
    switch (i)
    {
      case 0:	dir.d[0] = +2 * ONE; dir.d[1] = +0 * ONE; break;
      case 1:	dir.d[0] = +0 * ONE; dir.d[1] = +2 * ONE; break;
      case 2:	dir.d[0] = -2 * ONE; dir.d[1] = +0 * ONE; break;
      case 3:	dir.d[0] = +0 * ONE; dir.d[1] = -2 * ONE; break;
      case 4:	dir.d[0] = +2 * ONE; dir.d[1] = +2 * ONE; break;
      case 5:	dir.d[0] = -2 * ONE; dir.d[1] = +2 * ONE; break;
      case 6:	dir.d[0] = +2 * ONE; dir.d[1] = -2 * ONE; break;
      case 7:	dir.d[0] = -2 * ONE; dir.d[1] = -2 * ONE; break;
    }

    G_PushEntity(ent, &dir);

    // retry the original move
    ent->v.velocity.d[0] = oldvel->d[0];
    ent->v.velocity.d[1] = oldvel->d[1];
    ent->v.velocity.d[2] = 0;
    clip = G_FlyMove(ent, 41, &steptrace);

    if (oldorg.x == ent->v.origin.x || oldorg.y == ent->v.origin.y)
    {
      return clip;
    }

    // go back to the original pos and try again
    ent->v.origin = oldorg;
  }

  ent->v.velocity.x = 0;
  ent->v.velocity.y = 0;
  ent->v.velocity.z = 0;

  return 7; // still not moving
}

void G_WalkMove(edict_t *ent)
{
  x32vec3_t upmove = { 0 }, downmove = { 0 };
  x32vec3_t oldorg, oldvel;
  x32vec3_t nosteporg, nostepvel;
  u16 clip;
  u16 oldonground;
  const trace_t *steptrace, *downtrace;

  //
  // do a regular slide move unless it looks like you ran into a step
  //
  oldonground = ent->v.flags & FL_ONGROUND;
  ent->v.flags = ent->v.flags & ~FL_ONGROUND;

  oldorg = ent->v.origin;
  oldvel = ent->v.velocity;

  clip = G_FlyMove(ent, gs.frametime, &steptrace);

  if (!(clip & 2))
    return; // move didn't block on a step

  if (!oldonground && ent->v.waterlevel == 0)
    return; // don't stair up while jumping

  if (ent->v.movetype != MOVETYPE_WALK)
    return; // gibbed by a trigger

  if (ent->v.flags & FL_WATERJUMP)
    return;

  nosteporg = ent->v.origin;
  nostepvel = ent->v.velocity;

  //
  // try moving up and forward to go up a step
  //
  ent->v.origin = oldorg;	// back to start pos
  upmove.z = STEPSIZE;
  downmove.z = -STEPSIZE + xmul32(gs.frametime, oldvel.z);

  // move up
  G_PushEntity(ent, &upmove); // FIXME: don't link?

  // move forward
  ent->v.velocity.x = oldvel.x;
  ent->v.velocity.y = oldvel.y;
  ent->v.velocity.z = 0;
  clip = G_FlyMove(ent, gs.frametime, &steptrace);

  // check for stuckness, possibly due to the limited precision of floats
  // in the clipping hulls
  if (clip)
  {
    if (oldorg.y == ent->v.origin.y || oldorg.x == ent->v.origin.x)
      clip = G_TryUnstick(ent, &oldvel);
  }

  // extra friction based on view angle
  // if (clip & 2)
  //   G_WallFriction(ent, &steptrace);

  // move down
  downtrace = G_PushEntity(ent, &downmove); // FIXME: don't link?

  if (downtrace->plane.normal.z > 2867) // TO_FIX32(0.7)
  {
    if (ent->v.solid == SOLID_BSP)
    {
      ent->v.flags |= FL_ONGROUND;
      ent->v.groundentity = downtrace->ent;
    }
  }
  else
  {
    // if the push down didn't end up on good ground, use the move without
    // the step up.  This happens near wall / slope combinations, and can
    // cause the player to hop up higher on a slope too steep to climb
    ent->v.origin = nosteporg;
    ent->v.velocity = nostepvel;
  }
}
