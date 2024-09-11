#include <string.h>
#include "common.h"
#include "world.h"
#include "render.h"
#include "entity.h"
#include "game.h"
#include "g_phys.h"

game_state_t gs;

void G_StartMap(const char *path)
{
  memset(&gs, 0, sizeof(gs));

  gs.worldmodel = Mod_LoadModel(path);
  gs.num_edicts = 512;

  gs.models = Mem_ZeroAlloc((gs.worldmodel->numsubmodels + 1) * sizeof(*gs.models));
  gs.models[0] = gs.worldmodel;

  gs.edicts = Mem_ZeroAlloc(gs.num_edicts * sizeof(edict_t));

  // worldspawn
  gs.edicts[0].free = false;
  gs.edicts[0].v.solid = SOLID_BSP;
  gs.edicts[0].v.model = gs.worldmodel;
  gs.edicts[0].v.mins = gs.worldmodel->mins;
  gs.edicts[0].v.maxs = gs.worldmodel->maxs;

  // player 0
  gs.edicts[1].free = false;
  gs.edicts[1].v.solid = SOLID_BSP;
  gs.edicts[1].v.movetype = MOVETYPE_WALK;
  gs.edicts[1].v.origin = (x32vec3_t){ TO_FIX32(480), TO_FIX32(-352), TO_FIX32(88) };
  gs.edicts[1].v.mins = (x32vec3_t){ TO_FIX32(-16), TO_FIX32(-16), TO_FIX32(-24) };
  gs.edicts[1].v.maxs = (x32vec3_t){ TO_FIX32(16), TO_FIX32(16), TO_FIX32(32) };
  gs.player[0].ent = &gs.edicts[1];
  gs.player[0].viewofs.z = TO_FIX32(22);
  gs.player[0].viewangles.y = TO_DEG16(90);

  Sys_Printf("psb loaded, free mem: %u\n", Mem_GetFreeSpace());

  R_NewMap();
}

void G_PlayerMove(const x16 dt)
{
  x32vec3_t forwardmove = { 0 };
  x32vec3_t sidemove = { 0 };
  x32vec3_t upmove = { 0 };
  x16vec3_t vforward;
  x16vec3_t vright;
  x16vec3_t vup;
  player_state_t *plr = &gs.player[0];
  edict_t *ped = plr->ent;

  plr->viewangles.y += XMUL16(plr->anglemove.y, dt);
  plr->viewangles.x += XMUL16(plr->anglemove.x, dt);
  if (plr->viewangles.x < TO_DEG16(-89))
    plr->viewangles.x = TO_DEG16(-89);
  else if (plr->viewangles.x > TO_DEG16(89))
    plr->viewangles.x = TO_DEG16(89);

  ped->v.angles = plr->viewangles;
  ped->v.angles.z = 0;

  if (ped->v.movetype == MOVETYPE_WALK)
  {
    // gravity; velocity will get reset if we're on ground anyway
    ped->v.velocity.z -= XMUL16(dt, TO_FIX32(600));

    // on ground, decelerate and maybe jump
    if (ped->v.flags & FL_ONGROUND)
    {
      // extremely simple and stupid deceleration as well
      const x32 friction = XMUL16(dt, PLAYER_FRICTION);
      for (int i = 0; i < 2; ++i)
      {
        if (ped->v.velocity.d[i])
        {
          const s32 sign = (ped->v.velocity.d[i] < 0) ? -1 : 1;
          ped->v.velocity.d[i] -= sign * friction;
          if ((ped->v.velocity.d[i] < 0) ^ (sign < 0))
            ped->v.velocity.d[i] = 0;
        }
      }
      // jump
      if (plr->move.z > 0)
        ped->v.velocity.z = PLAYER_JUMP_SPEED;
    }

    // accelerate if pressing any directional buttons
    if (plr->move.x || plr->move.y)
    {
      // extremely simple and stupid acceleration for now
      AngleVectors(&ped->v.angles, &vforward, &vright, &vup);
      XVecScaleSL(&vforward, XMUL16(dt, plr->move.x), &forwardmove);
      XVecScaleSL(&vright, XMUL16(dt, plr->move.y), &sidemove);
      XVecAdd(&ped->v.velocity, &forwardmove, &ped->v.velocity);
      XVecAdd(&ped->v.velocity, &sidemove, &ped->v.velocity);
      // shitty rectangular clamp
      if (ped->v.velocity.x < -PLAYER_WALK_SPEED)
        ped->v.velocity.x = -PLAYER_WALK_SPEED;
      else if (ped->v.velocity.x > PLAYER_WALK_SPEED)
        ped->v.velocity.x = PLAYER_WALK_SPEED;
      if (ped->v.velocity.y < -PLAYER_WALK_SPEED)
        ped->v.velocity.y = -PLAYER_WALK_SPEED;
      else if (ped->v.velocity.y > PLAYER_WALK_SPEED)
        ped->v.velocity.y = PLAYER_WALK_SPEED;
    }

    // walk
    G_WalkMove(ped);
  }
  else if (ped->v.movetype == MOVETYPE_NOCLIP)
  {
    // fly around
    ped->v.velocity = x32vec3_origin;
    XVecScaleSL(&rs.vforward, XMUL16(dt, plr->move.x), &forwardmove);
    XVecScaleSL(&rs.vright, XMUL16(dt, plr->move.y), &sidemove);
    XVecScaleSL(&rs.vup, -XMUL16(dt, plr->move.z), &upmove);
    XVecAdd(&ped->v.origin, &forwardmove, &ped->v.origin);
    XVecAdd(&ped->v.origin, &sidemove, &ped->v.origin);
    XVecAdd(&ped->v.origin, &upmove, &ped->v.origin);
  }
}
