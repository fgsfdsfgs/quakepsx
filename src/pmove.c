#include "common.h"
#include "entity.h"
#include "game.h"
#include "move.h"
#include "system.h"

static x32 PM_UserFriction(const x32 stopspeed) {
  x32 speed, newspeed, control;
  x32vec3_t *vel = movevars->pm.velocity;
  x16vec3_t dir;

  // normally we could just divide newspeed by speed and get a scale factor
  // for our velocity; but divisions are a pain in the ass, so we get move
  // direction first and then just scale that

  XVecNormLS(vel, &dir, &speed);
  if (speed == 0)
    return 0;

  speed = TO_FIX32(SquareRoot0(speed));

  control = speed < stopspeed ? stopspeed : speed;
  control = xmul32(G_FRICTION, control);
  newspeed = speed - xmul32(gs.frametime, control);

  if (newspeed <= 0) {
    vel->x = 0;
    vel->y = 0;
    vel->z = 0;
  } else {
    XVecScaleSL(&dir, newspeed, vel);
  }

  return newspeed;
}

static void PM_Accelerate(void) {
  int i;
  x32 addspeed, accelspeed, currentspeed;

  currentspeed = XVecDotSL(&movevars->pm.wishdir, movevars->pm.velocity);
  addspeed = movevars->pm.wishspeed - currentspeed;
  if (addspeed <= 0)
    return;

  accelspeed = xmul32(gs.frametime, G_ACCELERATE);
  accelspeed = xmul32(accelspeed, movevars->pm.wishspeed);
  if (accelspeed > addspeed)
    accelspeed = addspeed;

  for (i = 0; i < 2; i++)
    movevars->pm.velocity->d[i] += xmul32(movevars->pm.wishdir.d[i], accelspeed);
}

static void PM_AirAccelerate(x32vec3_t *wishveloc) {
  int i;
  x32 addspeed, wishspd, accelspeed, currentspeed;

  wishspd = movevars->pm.wishspeed;
  if (wishspd > TO_FIX32(30))
    wishspd = TO_FIX32(30);

  currentspeed = XVecDotSL(&movevars->pm.wishdir, movevars->pm.velocity);
  addspeed = wishspd - currentspeed;
  if (addspeed <= 0)
    return;

  accelspeed = xmul32(gs.frametime, G_ACCELERATE);
  accelspeed = xmul32(accelspeed, wishspd);
  if (accelspeed > addspeed)
    accelspeed = addspeed;

  for (i = 0; i < 2; i++)
    movevars->pm.velocity->d[i] += xmul32(movevars->pm.wishdir.d[i], accelspeed);
}

static void PM_AirMove(void) {
  player_state_t *plr = &gs.player[0];
  x32vec3_t *wishvel = &movevars->pm.wishvel;
  x16vec3_t *wishdir = &movevars->pm.wishdir;
  x16vec3_t *right = &movevars->pm.right;
  x16vec3_t *up = &movevars->pm.up;
  x32 tmp, wishspeed, scale;

  // hack to not let you back into teleporter
  if (gs.time < plr->teleport_time && plr->move.x < 0)
    plr->move.x = 0;

  // player entity angles only have yaw
  AngleVectors(&plr->ent->v.angles, wishdir, right, up);

  for (int i = 0; i < 3; ++i)
    wishvel->d[i] = xmul32(wishdir->d[i], plr->move.x) + xmul32(right->d[i], plr->move.y);

  if (plr->ent->v.movetype != MOVETYPE_WALK)
    wishvel->z = plr->move.z;
  else
    wishvel->z = 0;

  wishspeed = XVecNormLS(wishvel, wishdir, &tmp);
  wishspeed = TO_FIX32(SquareRoot0(wishspeed));
  if (wishspeed > G_MAXSPEED) {
    scale = xdiv32(G_MAXSPEED, wishspeed);
    XVecScaleLS(wishvel, scale, wishvel);
    wishspeed = G_MAXSPEED;
  }

  movevars->pm.wishspeed = wishspeed;

  if (plr->ent->v.movetype == MOVETYPE_NOCLIP) {
    // noclip
    *movevars->pm.velocity = *wishvel;
  } else if (movevars->pm.onground) {
    PM_UserFriction(G_STOPSPEED);
    PM_Accelerate();
  } else {
    // not on ground, so little effect on velocity
    PM_AirAccelerate(wishvel);
  }
}

static void PM_WaterMove(void) {
  player_state_t *plr = &gs.player[0];
  x32vec3_t *wishvel = &movevars->pm.wishvel;
  x16vec3_t *forward = &movevars->pm.wishdir;
  x16vec3_t *right = &movevars->pm.right;
  x16vec3_t *up = &movevars->pm.up;
  x32 wishspeed, speed, newspeed, addspeed, accelspeed;
  x32 scale, tmp;
  int i;

  // user intentions
  AngleVectors(&plr->viewangles, forward, right, up);

  for (i = 0; i < 3; ++i)
    wishvel->d[i] = xmul32(forward->d[i], plr->move.x) + xmul32(right->d[i], plr->move.y);

  if (!plr->move.x && !plr->move.y && !plr->move.z)
    wishvel->z -= FTOX(60.0); // drift towards bottom
  else
    wishvel->z += plr->move.z;

  wishspeed = XVecLengthIntL(wishvel) << FIXSHIFT;
  if (wishspeed > G_MAXSPEED) {
    scale = xdiv32(G_MAXSPEED, wishspeed);
    wishvel->x = xmul32(scale, wishvel->x);
    wishvel->y = xmul32(scale, wishvel->y);
    wishvel->z = xmul32(scale, wishvel->z);
    wishspeed = G_MAXSPEED;
  }
  wishspeed = xmul32(wishspeed, FTOX(0.7));

  // water friction
  speed = XVecLengthIntL(movevars->pm.velocity) << FIXSHIFT;
  if (speed) {
    newspeed = speed - xmul32(gs.frametime, xmul32(speed, G_FRICTION));
    if (newspeed < 0)
      newspeed = 0;
    scale = xdiv32(newspeed, speed);
    movevars->pm.velocity->x = xmul32(scale, movevars->pm.velocity->x);
    movevars->pm.velocity->y = xmul32(scale, movevars->pm.velocity->y);
    movevars->pm.velocity->z = xmul32(scale, movevars->pm.velocity->z);
  } else {
    newspeed = 0;
  }

  // water acceleration
  if (!wishspeed)
    return;

  addspeed = wishspeed - newspeed;
  if (addspeed <= 0)
    return;

  XVecNormLS(wishvel, forward, &tmp);

  accelspeed = xmul32(gs.frametime, xmul32(wishspeed, G_ACCELERATE));
  if (accelspeed > addspeed)
    accelspeed = addspeed;

  for (i = 0; i < 3; ++i)
    movevars->pm.velocity->d[i] += xmul32(forward->d[i], accelspeed);
}

static inline void PM_WaterJump (void) {
  player_state_t *plr = &gs.player[0];
  edict_t *ent = plr->ent;
  if (gs.time > plr->teleport_time || !ent->v.waterlevel) {
    ent->v.flags &= ~FL_WATERJUMP;
    plr->teleport_time = 0;
  }
  ent->v.velocity.x = plr->waterjump.x;
  ent->v.velocity.y = plr->waterjump.y;
}

void PM_PlayerMove(const x16 dt) {
  x32vec3_t forwardmove = { 0 };
  player_state_t *plr = &gs.player[0];
  edict_t *ped = plr->ent;

  plr->viewangles.y += XMUL16(plr->anglemove.y, dt);
  plr->viewangles.x += XMUL16(plr->anglemove.x, dt);
  if (plr->viewangles.x < TO_DEG16(-89))
    plr->viewangles.x = TO_DEG16(-89);
  else if (plr->viewangles.x > TO_DEG16(89))
    plr->viewangles.x = TO_DEG16(89);

  movevars->pm.onground = (ped->v.flags & FL_ONGROUND) != 0;
  movevars->pm.origin = &ped->v.origin;
  movevars->pm.velocity = &ped->v.velocity;

  if (ped->v.movetype == MOVETYPE_WALK) {
    if (ped->v.flags & FL_WATERJUMP) {
      PM_WaterJump();
    } else if (ped->v.waterlevel >= 2) {
      // calculates its own wishdir
      PM_WaterMove();
    } else {
      PM_AirMove();
      // jump if able
      if (movevars->pm.onground && plr->move.z > 0) {
        ped->v.velocity.z += G_JUMPSPEED;
        plr->flags |= PFL_JUMPED;
      }
    }
  } else if (ped->v.movetype == MOVETYPE_NOCLIP) {
    // fly around
    PM_AirMove();
    XVecScaleLS(&ped->v.velocity, dt, &forwardmove);
    XVecAdd(&ped->v.origin, &forwardmove, &ped->v.origin);
  }
}
