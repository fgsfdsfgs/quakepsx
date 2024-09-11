#include "common.h"
#include "world.h"
#include "render.h"
#include "entity.h"
#include "game.h"

#define MAX_CLIP_PLANES 3
#define STOP_EPSILON 256

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

  backoff = xmul32(overbounce, XVecDot16x32(normal, in));

  for (i = 0; i < 3; i++)
  {
    change = xmul32(normal->d[i], backoff);
    out->d[i] = in->d[i] - change;
    if (out->d[i] > -STOP_EPSILON && out->d[i] < STOP_EPSILON)
      out->d[i] = 0;
  }

  return blocked;
}

int G_FlyMove(edict_t *ent, x16 time, const trace_t **steptrace)
{
  int bumpcount;
  x16vec3_t dir;
  x32 d;
  int numplanes;
  x16vec3_t planes[MAX_CLIP_PLANES];
  x32vec3_t primal_velocity, original_velocity, new_velocity;
  int i, j;
  const trace_t *trace;
  x32vec3_t end;
  x16 time_left;
  int blocked;

  blocked = 0;
  original_velocity = ent->v.velocity;
  primal_velocity = ent->v.velocity;
  numplanes = 0;

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
          if (XVecDot16x32(&planes[j], &new_velocity) < 0)
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
      XVecCross16(&planes[0], &planes[1], &dir);
      d = XVecDot16x32(&dir, &ent->v.velocity);
      XVecScaleSL(&dir, d, &ent->v.velocity);
    }

    //
    // if original velocity is against the original velocity, stop dead
    // to avoid tiny occilations in sloping corners
    //
    if (XVecDot32x32(&ent->v.velocity, &primal_velocity) <= 0)
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

  return trace;
}

#define STEPSIZE TO_FIX32(18)
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
    // TODO
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
      // ent->v.groundentity = EDICT_TO_PROG(downtrace->ent);
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
