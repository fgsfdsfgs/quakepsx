#include "common.h"
#include "world.h"
#include "entity.h"
#include "game.h"
#include "move.h"
#include "system.h"

#define STOP_EPSILON 41 // ~0.01

static void PM_UserFriction(void)
{
  x32 speed, newspeed, control;
  x32vec3_t *vel = movevars->pm.velocity;
  x16vec3_t dir;

  // normally we could just divide newspeed by speed and get a scale factor
  // for our velocity; but divisions are a pain in the ass, so we get move
  // direction first and then just scale that

  XVecNormLS(vel, &dir, &speed);
  if (speed < STOP_EPSILON)
  {
    vel->x = 0;
    vel->y = 0;
    return;
  }

  speed = TO_FIX32(SquareRoot0(speed));

  control = speed < G_STOPSPEED ? G_STOPSPEED : speed;
  control = xmul32(G_FRICTION, control);
  newspeed = speed - xmul32(gs.frametime, control);

  if (newspeed < STOP_EPSILON)
  {
    vel->x = 0;
    vel->y = 0;
    vel->z = 0;
  }
  else
  {
    XVecScaleSL(&dir, newspeed, vel);
  }
}

static void PM_Accelerate(void)
{
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

static void PM_AirAccelerate(x32vec3_t *wishveloc)
{
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

static void PM_AirMove(void)
{
  player_state_t *plr = &gs.player[0];
  x32vec3_t *wishvel = &movevars->pm.wishvel;

  // assume movespeed is either G_FORWARDSPEED or 2 * G_FORWARDSPEED
  wishvel->x = xmul32(movevars->pm.wishdir.x, plr->movespeed);
  wishvel->y = xmul32(movevars->pm.wishdir.y, plr->movespeed);

  if (plr->ent->v.movetype != MOVETYPE_WALK)
    wishvel->z = plr->move.z;
  else
    wishvel->z = 0;

  movevars->pm.wishspeed = plr->movespeed;

  if (movevars->pm.wishspeed > G_MAXSPEED)
  {
    // this can only happen if we're sprinting, and that doubles the speed
    // so the fraction would be 320 / (2 * 200) = 0.8
    XVecScaleLS(wishvel, G_DOUBLESPEEDFRAC, wishvel);
    movevars->pm.wishspeed = G_MAXSPEED;
  }

  if (plr->ent->v.movetype == MOVETYPE_NOCLIP)
  {
    // noclip
    *movevars->pm.velocity = *wishvel;
  }
  else if (movevars->pm.onground)
  {
    PM_UserFriction();
    PM_Accelerate();
  }
  else
  {
    // not on ground, so little effect on velocity
    PM_AirAccelerate(wishvel);
  }
}

static inline void CalculateWishDir(const x32vec3_t *move, const x16 yaw, x16vec3_t *wishdir)
{
  x16 wx;
  x16 wy;

  wishdir->z = 0;

  if (move->x && move->y)
  {
    // diagonal move, vector is gonna be (+-1/sqrt(2), +-1/sqrt(2), 0)
    wx = xsign32(move->x) * 2896;
    wy = -xsign32(move->y) * 2896;
  }
  else if (move->x || move->y)
  {
    // cardinal move, vector is gonna be either (+-1, 0, 0) or (0, +-1, 0)
    wx = xsign32(move->x) * ONE;
    wy = -xsign32(move->y) * ONE;
  }
  else
  {
    // no move
    wishdir->x = wishdir->y = 0;
    return;
  }

  // rotate wishdir
  const x32 sy = isin(yaw);
  const x32 cy = icos(yaw);
  wishdir->x = XMUL16(wx, cy) - XMUL16(wy, sy);
  wishdir->y = XMUL16(wx, sy) + XMUL16(wy, cy);
}

void PM_PlayerMove(const x16 dt)
{
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

  // when our yaw is zero, forward vector is (1, 0, 0)
  // we can calculate the direction in which we want to go, then rotate it
  // this is needed to avoid having to normalize vectors and calculate their lengths
  x16vec3_t *wishdir = &movevars->pm.wishdir;
  CalculateWishDir(&plr->move, ped->v.angles.y, wishdir);

  if (ped->v.movetype == MOVETYPE_WALK)
  {
    // calculate velocity
    PM_AirMove();

    // jump if able, otherwise apply gravity
    if (movevars->pm.onground && plr->move.z > 0)
      ped->v.velocity.z += G_JUMPSPEED;

    ped->v.velocity.z -= xmul32(dt, G_GRAVITY);

    // apply movement
    G_WalkMove(ped);
  }
  else if (ped->v.movetype == MOVETYPE_NOCLIP)
  {
    // fly around
    PM_AirMove();
    XVecScaleLS(&ped->v.velocity, dt, &forwardmove);
    XVecAdd(&ped->v.origin, &forwardmove, &ped->v.origin);
  }
}
