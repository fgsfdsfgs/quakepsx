#include <string.h>

#include "common.h"
#include "system.h"
#include "world.h"
#include "entity.h"
#include "game.h"
#include "sound.h"
#include "progs.h"
#include "move.h"

movevars_t *const movevars = PSX_SCRATCH;

#define MAX_CLIP_PLANES 4
#define STOP_EPSILON 256
#define MAX_PUSHED 64

static int ClipVelocity(x32vec3_t *in, const x16vec3_t *normal, x32vec3_t *out, const x16 overbounce) {
  x32 backoff;
  x32 change;
  int i, blocked;

  blocked = 0;
  if (normal->z > 0)
    blocked |= 1; // floor
  else if (!normal->z)
    blocked |= 2; // step

  backoff = xmul32(overbounce, XVecDotSL(normal, in));

  for (i = 0; i < 3; i++) {
    change = xmul32(normal->d[i], backoff);
    out->d[i] = in->d[i] - change;
    if (out->d[i] > -STOP_EPSILON && out->d[i] < STOP_EPSILON)
      out->d[i] = 0;
  }

  return blocked;
}

static inline void Impact(edict_t *e1, edict_t *e2) {
  if (e1->v.touch && e1->v.solid != SOLID_NOT)
    e1->v.touch(e1, e2);
  if (e2->v.touch && e2->v.solid != SOLID_NOT)
    e2->v.touch(e2, e1);
}

static inline void AddGravity(edict_t *ent) {
  // TODO: ent gravity
  ent->v.velocity.z -= xmul32(gs.frametime, G_GRAVITY);
}

static void ClipMoveToEntity(edict_t *ent, x32vec3_t *start, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *end, trace_t *trace) {
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

static qboolean CheckWater(edict_t *ent) {
  x32vec3_t point;
  int cont;

  point.d[0] = ent->v.origin.d[0];
  point.d[1] = ent->v.origin.d[1];
  point.d[2] = ent->v.origin.d[2] + ent->v.mins.d[2] + ONE;

  ent->v.waterlevel = 0;
  ent->v.watertype = CONTENTS_EMPTY;
  cont = G_PointContents(&point);
  if (cont <= CONTENTS_WATER) {
    ent->v.watertype = cont;
    ent->v.waterlevel = 1;
    point.d[2] = ent->v.origin.d[2] + ((ent->v.mins.d[2] + ent->v.maxs.d[2]) >> 1);
    cont = G_PointContents(&point);
    if (cont <= CONTENTS_WATER) {
      ent->v.waterlevel = 2;
      point.d[2] = ent->v.origin.d[2] + ent->v.viewheight;
      cont = G_PointContents(&point);
      if (cont <= CONTENTS_WATER)
        ent->v.waterlevel = 3;
    }
  }

  return ent->v.waterlevel > 1;
}

static void CheckWaterTransition(edict_t *ent) {
  int cont;
  cont = G_PointContents(&ent->v.origin);

  if (!ent->v.watertype) {
    // just spawned here
    ent->v.watertype = cont;
    ent->v.waterlevel = 1;
    return;
  }

  if (cont <= CONTENTS_WATER) {
    if (ent->v.watertype == CONTENTS_EMPTY) {
      // just crossed into water
      // TODO: StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);
    }
    ent->v.watertype = cont;
    ent->v.waterlevel = 1;
  } else {
    if (ent->v.watertype != CONTENTS_EMPTY) {
      // just crossed into water
      // TODO: StartSound (ent, 0, "misc/h2ohit1.wav", 255, 1);
    }
    ent->v.watertype = CONTENTS_EMPTY;
    ent->v.waterlevel = cont;
  }
}

qboolean G_RunThink(edict_t *ent) {
  x32 thinktime = ent->v.nextthink;
  if (thinktime <= 0 || thinktime > gs.time + gs.frametime || !ent->v.think)
    return true;

  if (thinktime < gs.time) {
    // don't let things stay in the past.
    // it is possible to start that way
    // by a trigger with a local time.
    thinktime = gs.time;
  }

  const x32 oldtime = gs.time;
  ent->v.nextthink = 0;
  gs.time = thinktime;
  ent->v.think(ent);
  gs.time = oldtime;

  return !ent->free;
}

void G_ClipToLinks(areanode_t *node, moveclip_t *clip) {
  link_t *l, *next;
  edict_t *touch;
  trace_t *trace = &clip->tmptrace;

  // touch linked edicts
  for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next) {
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
    || clip->boxmaxs.d[2] < touch->v.absmin.d[2])
      continue;

    if (clip->passedict && clip->passedict->v.size.x && !touch->v.size.x)
      continue; // points never interact

    // might intersect, so do an exact clip
    if (clip->trace.allsolid)
      return;
    if (clip->passedict) {
      if (touch->v.owner == clip->passedict)
        continue; // don't clip against own missiles
      if (clip->passedict->v.owner == touch)
        continue; // don't clip against owner
    }

    if (touch->v.flags & FL_MONSTER)
      ClipMoveToEntity(touch, clip->start, &clip->mins2, &clip->maxs2, clip->end, &clip->tmptrace);
    else
      ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end, &clip->tmptrace);

    if (trace->allsolid || trace->startsolid || trace->fraction < clip->trace.fraction) {
      trace->ent = touch;
      if (clip->trace.startsolid) {
        clip->trace = *trace;
        clip->trace.startsolid = true;
      } else {
        clip->trace = *trace;
      }
    } else if (trace->startsolid) {
      clip->trace.startsolid = true;
    }
  }

  // recurse down both sides
  if (node->axis == -1)
    return;

  if (clip->boxmaxs.d[node->axis] > node->dist)
    G_ClipToLinks(node->children[0], clip);
  if (clip->boxmins.d[node->axis] < node->dist)
    G_ClipToLinks(node->children[1], clip);
}

const trace_t *G_Move(x32vec3_t *start, x32vec3_t *mins, x32vec3_t *maxs, x32vec3_t *end, int type, edict_t *passedict) {
  moveclip_t *clip = &movevars->clip;
  int i;

  memset(clip, 0, sizeof(moveclip_t));

  // clip to world
  ClipMoveToEntity(gs.edicts, start, mins, maxs, end, &clip->trace);

  clip->start = start;
  clip->end = end;
  clip->mins = mins;
  clip->maxs = maxs;
  clip->type = type;
  clip->passedict = passedict;

  if (type == MOVE_MISSILE) {
    for (i = 0; i < 3; i++) {
      clip->mins2.d[i] = -TO_FIX32(15);
      clip->maxs2.d[i] = TO_FIX32(15);
    }
  } else {
    clip->mins2 = *mins;
    clip->maxs2 = *maxs;
  }

  // create the bounding box of the entire move
  G_MoveBounds(start, &clip->mins2, &clip->maxs2, end, &clip->boxmins, &clip->boxmaxs);

  // clip to entities
  G_ClipToLinks(g_areanodes, clip);

  return &clip->trace;
}

edict_t *G_TestEntityPosition(edict_t *ent) {
  const trace_t *trace;
  trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &ent->v.origin, 0, ent);
  if (trace->startsolid)
    return gs.edicts;
  return NULL;
}

const trace_t *G_PushEntity(edict_t *ent, x32vec3_t *push) {
  const trace_t *trace;
  x32vec3_t end;

  XVecAdd(&ent->v.origin, push, &end);

  if (ent->v.movetype == MOVETYPE_FLYMISSILE)
    trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, MOVE_MISSILE, ent);
  else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
    // only clip against bmodels
    trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, MOVE_NOMONSTERS, ent);
  else
    trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, MOVE_NORMAL, ent);

  ent->v.origin = trace->endpos;
  G_LinkEdict(ent, true);

  if (trace->ent)
    Impact(ent, trace->ent);

  return trace;
}

int G_TryUnstick(edict_t *ent, x32vec3_t *oldvel) {
  int i;
  x32vec3_t oldorg = ent->v.origin;
  x32vec3_t dir = { 0 };
  int clip;
  const trace_t *steptrace;

  for (i = 0; i < 8; i++) {
    // try pushing a little in an axial direction
    switch (i) {
    case 0: dir.d[0] = +2 * ONE; dir.d[1] = +0 * ONE; break;
    case 1: dir.d[0] = +0 * ONE; dir.d[1] = +2 * ONE; break;
    case 2: dir.d[0] = -2 * ONE; dir.d[1] = +0 * ONE; break;
    case 3: dir.d[0] = +0 * ONE; dir.d[1] = -2 * ONE; break;
    case 4: dir.d[0] = +2 * ONE; dir.d[1] = +2 * ONE; break;
    case 5: dir.d[0] = -2 * ONE; dir.d[1] = +2 * ONE; break;
    case 6: dir.d[0] = +2 * ONE; dir.d[1] = -2 * ONE; break;
    case 7: dir.d[0] = -2 * ONE; dir.d[1] = -2 * ONE; break;
    }

    G_PushEntity(ent, &dir);

    // retry the original move
    ent->v.velocity.d[0] = oldvel->d[0];
    ent->v.velocity.d[1] = oldvel->d[1];
    ent->v.velocity.d[2] = 0;
    clip = G_FlyMove(ent, 41, &steptrace);

    if (oldorg.x == ent->v.origin.x || oldorg.y == ent->v.origin.y)
      return clip;

    // go back to the original pos and try again
    ent->v.origin = oldorg;
  }

  ent->v.velocity.x = 0;
  ent->v.velocity.y = 0;
  ent->v.velocity.z = 0;

  return 7; // still not moving
}

int G_FlyMove(edict_t *ent, x16 time, const trace_t **steptrace) {
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

  for (bumpcount = 0; bumpcount < 3; bumpcount++) {
    if (!ent->v.velocity.x && !ent->v.velocity.y && !ent->v.velocity.z)
      break;

    XVecScaleLS(&ent->v.velocity, time_left, &end);
    XVecAdd(&ent->v.origin, &end, &end);

    trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, false, ent);

    if (trace->allsolid) {
      // entity is trapped in another solid
      XVecZero(&ent->v.velocity);
      return 3;
    }

    if (trace->fraction > 0) {
      // actually covered some distance
      ent->v.origin = trace->endpos;
      original_velocity = ent->v.velocity;
      numplanes = 0;
    }

    if (trace->fraction == ONE)
        break; // moved the entire distance

    if (trace->plane.normal.z > G_FLOORNORMALZ) {
      blocked |= 1;  // floor
      if (trace->ent->v.solid == SOLID_BSP) {
        ent->v.flags |= FL_ONGROUND;
        ent->v.groundentity = trace->ent;
      }
    } else if (!trace->plane.normal.z) {
      blocked |= 2; // step
      if (steptrace)
        *steptrace = trace; // save for player extrafriction
    }

    // run the impact function
    Impact(ent, trace->ent);
    if (ent->free)
      break; // removed by the impact function

    time_left -= XMUL16(time_left, trace->fraction);

    planes[numplanes] = trace->plane.normal;
    numplanes++;

    // modify original_velocity so it parallels all of the clip planes
    for (i = 0; i < numplanes; i++) {
      ClipVelocity(&original_velocity, &planes[i], &new_velocity, ONE);
      for (j = 0; j < numplanes; j++) {
        if (j != i) {
          if (XVecDotSL(&planes[j], &new_velocity) < 0)
            break; // not ok
        }
      }
      if (j == numplanes)
        break;
    }

    if (i != numplanes) {
      // go along this plane
      ent->v.velocity = new_velocity;
    } else {
      // go along the crease
      if (numplanes != 2) {
        XVecZero(&ent->v.velocity);
        return 7;
      }
      XVecCrossSS(&planes[0], &planes[1], &dir);
      d = XVecDotSL(&dir, &ent->v.velocity);
      XVecScaleSL(&dir, d, &ent->v.velocity);
    }

    // if original velocity is against the original velocity, stop dead
    // to avoid tiny occilations in sloping corners
    if (XVecDotSL(&original_signs, &ent->v.velocity) <= 0) {
      XVecZero(&ent->v.velocity);
      return blocked;
    }
  }

  return blocked;
}

void G_WalkMove(edict_t *ent) {
  x32vec3_t upmove = { 0 }, downmove = { 0 };
  x32vec3_t oldorg, oldvel;
  x32vec3_t nosteporg, nostepvel;
  u16 clip;
  u16 oldonground;
  const trace_t *steptrace, *downtrace;

  // do a regular slide move unless it looks like you ran into a step
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

  // try moving up and forward to go up a step
  ent->v.origin = oldorg; // back to start pos
  upmove.z = G_STEPSIZE;
  downmove.z = -G_STEPSIZE + xmul32(gs.frametime, oldvel.z);

  // move up
  G_PushEntity(ent, &upmove); // FIXME: don't link?

  // move forward
  ent->v.velocity.x = oldvel.x;
  ent->v.velocity.y = oldvel.y;
  ent->v.velocity.z = 0;
  clip = G_FlyMove(ent, gs.frametime, &steptrace);

  // check for stuckness, possibly due to the limited precision of floats
  // in the clipping hulls
  if (clip) {
    if (oldorg.y == ent->v.origin.y || oldorg.x == ent->v.origin.x)
      clip = G_TryUnstick(ent, &oldvel);
  }

  // extra friction based on view angle
  // if (clip & 2)
  //   G_WallFriction(ent, &steptrace);

  // move down
  downtrace = G_PushEntity(ent, &downmove); // FIXME: don't link?

  if (downtrace->plane.normal.z > G_FLOORNORMALZ) {
    if (ent->v.solid == SOLID_BSP) {
      ent->v.flags |= FL_ONGROUND;
      ent->v.groundentity = downtrace->ent;
    }
  } else {
    // if the push down didn't end up on good ground, use the move without
    // the step up.  This happens near wall / slope combinations, and can
    // cause the player to hop up higher on a slope too steep to climb
    ent->v.origin = nosteporg;
    ent->v.velocity = nostepvel;
  }
}

void G_PushMove(edict_t *pusher, x16 movetime) {
  x32vec3_t entorig;
  x32vec3_t move;
  x32vec3_t mins, maxs;
  x32vec3_t pushorig;
  x32vec3_t moved_orig[MAX_PUSHED];
  edict_t *moved_edict[MAX_PUSHED];
  int num_moved;
  edict_t *check;

  if (!pusher->v.velocity.x && !pusher->v.velocity.y && !pusher->v.velocity.z) {
    pusher->v.ltime += movetime;
    return;
  }

  for (int i = 0; i < 3; ++i) {
    move.d[i] = xmul32(movetime, pusher->v.velocity.d[i]);
    mins.d[i] = pusher->v.absmin.d[i] + move.d[i];
    maxs.d[i] = pusher->v.absmax.d[i] + move.d[i];
  }

  pushorig = pusher->v.origin;

  // move the pusher to it's final position
  XVecAdd(&pusher->v.origin, &move, &pusher->v.origin);
  pusher->v.ltime += movetime;
  G_LinkEdict(pusher, false);

  // see if any solid entities are inside the final position
  num_moved = 0;
  check = gs.edicts + 1;
  for (int i = 1; i <= gs.max_edict; ++i, ++check) {
    if (check->free)
      continue;

    if (check->v.movetype == MOVETYPE_PUSH
    || check->v.movetype == MOVETYPE_NONE
    || check->v.movetype == MOVETYPE_NOCLIP)
      continue;

    // if the entity is standing on the pusher, it will definately be moved
    if (!((check->v.flags & FL_ONGROUND) && check->v.groundentity == pusher)) {
      if (check->v.absmin.d[0] >= maxs.d[0]
      || check->v.absmin.d[1] >= maxs.d[1]
      || check->v.absmin.d[2] >= maxs.d[2]
      || check->v.absmax.d[0] <= mins.d[0]
      || check->v.absmax.d[1] <= mins.d[1]
      || check->v.absmax.d[2] <= mins.d[2])
        continue;
      // see if the ent's bbox is inside the pusher's final position
      if (!G_TestEntityPosition(check))
        continue;
    }

    // remove the onground flag for non-players
    if (check->v.movetype != MOVETYPE_WALK)
      check->v.flags &= ~FL_ONGROUND;

    if (num_moved >= MAX_PUSHED)
      Sys_Error("G_PushMove: pushed too many entities\n");

    entorig = check->v.origin;
    moved_orig[num_moved] = entorig;
    moved_edict[num_moved] = check;
    ++num_moved;

    // try moving the contacted entity
    pusher->v.solid = SOLID_NOT;
    G_PushEntity(check, &move);
    pusher->v.solid = SOLID_BSP;

    // if it is still inside the pusher, block
    if (G_TestEntityPosition(check)) {
      // fail the move
      if (check->v.mins.x == check->v.maxs.x)
        continue;
      if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER) {
        // corpse
        check->v.mins.x = check->v.mins.y = 0;
        check->v.maxs = check->v.mins;
        continue;
      }

      check->v.origin = entorig;
      G_LinkEdict(check, true);

      pusher->v.origin = pushorig;
      G_LinkEdict(pusher, false);
      pusher->v.ltime -= movetime;

      // if the pusher has a "blocked" function, call it
      // otherwise, just stay in place until the obstacle is gone
      if (pusher->v.blocked)
        pusher->v.blocked(pusher, check);
      // move back any entities we already moved
      for (int j = 0; j < num_moved; j++) {
        moved_edict[j]->v.origin = moved_orig[j];
        G_LinkEdict(moved_edict[j], false);
      }
      return;
    }
  }
}

qboolean G_DropToFloor(edict_t *ent) {
  x32vec3_t end = ent->v.origin;
  end.z -= TO_FIX32(256);

  const trace_t *trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &end, false, ent);
  if (trace->fraction == ONE || trace->allsolid) {
    return false;
  } else {
    ent->v.origin = trace->endpos;
    G_LinkEdict(ent, false);
    ent->v.flags |= FL_ONGROUND;
    ent->v.groundentity = trace->ent;
    return true;
  }
}

qboolean G_CheckBottom(edict_t *ent) {
  x32vec3_t mins, maxs, start, stop;
  const trace_t *trace;
  int x, y;
  x32 mid, bottom;

  XVecAdd(&ent->v.origin, &ent->v.mins, &mins);
  XVecAdd(&ent->v.origin, &ent->v.maxs, &maxs);

  // if all of the points under the corners are solid world, don't bother
  // with the tougher checks
  // the corners must be within 16 of the midpoint
  start.z = mins.z - ONE;
  for (x = 0; x <= 1; x++) {
    for (y = 0; y <= 1; y++) {
      start.x = x ? maxs.x : mins.x;
      start.y = y ? maxs.y : mins.y;
      if (G_PointContents(&start) != CONTENTS_SOLID)
        goto realcheck;
    }
  }

  // we got out easy
  return true;

realcheck:
  // check it for real...
  start.z = mins.z;

  // the midpoint must be within 16 of the bottom
  start.x = stop.x = (mins.x + maxs.x) >> 1;
  start.y = stop.y = (mins.y + maxs.y) >> 1;
  stop.z = start.z - 2 * G_STEPSIZE;
  trace = G_Move(&start, &x32vec3_origin, &x32vec3_origin, &stop, true, ent);

  if (trace->fraction == ONE)
    return false;

  mid = bottom = trace->endpos.z;

  // the corners must be within 16 of the midpoint
  for (x = 0; x <= 1; x++) {
    for (y = 0; y <= 1; y++) {
      start.x = stop.x = x ? maxs.x : mins.x;
      start.y = stop.y = y ? maxs.y : mins.y;
      trace = G_Move(&start, &x32vec3_origin, &x32vec3_origin, &stop, true, ent);
      if (trace->fraction != ONE && trace->endpos.z > bottom)
        bottom = trace->endpos.z;
      if (trace->fraction == ONE || mid - trace->endpos.z > G_STEPSIZE)
        return false;
    }
  }

  return true;
}

static inline void PhysicsNone(edict_t *ent) {
  // regular thinking
  G_RunThink(ent);
}

static inline void PhysicsPusher(edict_t *ent) {
  x32 oldltime = ent->v.ltime;
  x32 thinktime = ent->v.nextthink;
  x16 movetime;

  thinktime = ent->v.nextthink;
  if (thinktime < ent->v.ltime + gs.frametime) {
    movetime = thinktime - ent->v.ltime;
    if (movetime < 0)
      movetime = 0;
  } else {
    movetime = gs.frametime;
  }

  if (movetime)
    G_PushMove(ent, movetime); // advances ent->v.ltime if not blocked

  if (thinktime > oldltime && thinktime <= ent->v.ltime && ent->v.think) {
    ent->v.nextthink = 0;
    ent->v.think(ent);
  }
}

static inline void PhysicsNoclip(edict_t *ent) {
  // regular thinking
  if (!G_RunThink(ent))
    return;

  XVecMulAddS(&ent->v.angles, gs.frametime, &ent->v.avelocity, &ent->v.angles);
  XVecMulAddLSL(&ent->v.origin, gs.frametime, &ent->v.velocity, &ent->v.origin);

  G_LinkEdict(ent, false);
}

static void PhysicsStep(edict_t *ent) {
  qboolean hitsound;

  // freefall if not onground
  if (!(ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
    if (ent->v.velocity.d[2] < (G_GRAVITY / -10))
      hitsound = true;
    else
      hitsound = false;

    AddGravity(ent);
    // CheckVelocity(ent);
    G_FlyMove(ent, gs.frametime, NULL);
    G_LinkEdict(ent, true);

    if (ent->v.flags & FL_ONGROUND) {
      // just hit ground
      // if (hitsound)
      //   TODO: StartSound(ent, 0, "demon/dland2.wav", 255, 1);
    }
  }

  // regular thinking
  G_RunThink(ent);

  CheckWaterTransition(ent);
}

static void PhysicsToss(edict_t *ent) {
  x32vec3_t move;
  const trace_t *trace;
  x16 backoff;

  // regular thinking
  if (!G_RunThink(ent))
    return;

  // if onground, return without moving
  if ((ent->v.flags & FL_ONGROUND))
    return;

  // add gravity
  if (ent->v.movetype != MOVETYPE_FLY && ent->v.movetype != MOVETYPE_FLYMISSILE)
    AddGravity(ent);

  // move angles
  XVecMulAddS(&ent->v.angles, gs.frametime, &ent->v.avelocity, &ent->v.angles);

  // move origin
  XVecScaleSL(&ent->v.velocity, gs.frametime, &move);
  trace = G_PushEntity(ent, &move);
  if (ent->free || trace->fraction == ONE)
    return;

  backoff = (ent->v.movetype == MOVETYPE_BOUNCE) ? (ONE + HALF) : ONE;
  ClipVelocity(&ent->v.velocity, &trace->plane.normal, &ent->v.velocity, backoff);

  // stop if on ground
  if (trace->plane.normal.z > G_FLOORNORMALZ) {
    if (ent->v.velocity.z < TO_FIX32(60) || ent->v.movetype != MOVETYPE_BOUNCE) {
      ent->v.flags |= FL_ONGROUND;
      ent->v.groundentity = trace->ent;
      XVecZero(&ent->v.velocity);
      XVecZero(&ent->v.avelocity);
    }
  }

  // check for in water
  CheckWaterTransition(ent);
}

static void PhysicsPlayer(edict_t *ent) {
  ent->v.oldorigin = ent->v.origin;

  // call standard client pre-think
  Player_PreThink(ent);

  PM_PlayerMove(gs.frametime);

  // do a move
  // TODO: CheckVelocity?
  // decide which move function to call
  switch (ent->v.movetype) {
  case MOVETYPE_NONE:
    if (!G_RunThink(ent))
      return;
    break;
  case MOVETYPE_WALK:
    if (!G_RunThink(ent))
      return;
    if (!CheckWater(ent) && !(ent->v.flags & FL_WATERJUMP))
      AddGravity(ent);
    // CheckStuck(ent);
    G_WalkMove(ent);
    break;

  case MOVETYPE_TOSS:
  case MOVETYPE_BOUNCE:
    PhysicsToss(ent);
    break;

  case MOVETYPE_FLY:
    if (!G_RunThink(ent))
      return;
    G_FlyMove(ent, gs.frametime, NULL);
    break;

  case MOVETYPE_NOCLIP:
    if (!G_RunThink(ent))
      return;
    XVecMulAddLSL(&ent->v.origin, gs.frametime, &ent->v.velocity, &ent->v.origin);
    break;

  default:
    // should not happen
    break;
  }

  // call standard player post-think
  G_LinkEdict(ent, true);
  Player_PostThink(ent);
}

void G_Physics(void) {
  int i;
  edict_t *ent;

  // let the progs know that a new frame has started
  // TODO

  //
  // treat each object in turn
  //
  ent = gs.edicts + 1;
  for (i = 1; i <= gs.max_edict; ++i, ++ent) {
    if (ent->free)
      continue;

    if (gs.force_retouch)
      G_LinkEdict(ent, true); // force retouch even for stationary

    ent->v.oldorigin = ent->v.origin;

    if (i == 1)
      PhysicsPlayer(ent);
    else
      switch (ent->v.movetype) {
      case MOVETYPE_NONE:
        PhysicsNone(ent);
        break;
      case MOVETYPE_PUSH:
        PhysicsPusher(ent);
        break;
      case MOVETYPE_NOCLIP:
        PhysicsNoclip(ent);
        break;
      case MOVETYPE_STEP:
        PhysicsStep(ent);
        break;
      case MOVETYPE_TOSS:
      case MOVETYPE_BOUNCE:
      case MOVETYPE_FLY:
      case MOVETYPE_FLYMISSILE:
        PhysicsToss(ent);
      default:
        break;
      }
  }

  if (gs.force_retouch)
    gs.force_retouch--;

  gs.time += gs.frametime;
}
