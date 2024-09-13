#include <string.h>
#include "common.h"
#include "world.h"
#include "render.h"
#include "entity.h"
#include "game.h"
#include "move.h"

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

void G_Update(const x16 dt)
{
  x32vec3_t noclipvel;
  player_state_t *plr = &gs.player[0];
  edict_t *ped = plr->ent;

  plr->viewangles.y += XMUL16(plr->anglemove.y, dt);
  plr->viewangles.x += XMUL16(plr->anglemove.x, dt);
  if (plr->viewangles.x < TO_DEG16(-89))
    plr->viewangles.x = TO_DEG16(-89);
  else if (plr->viewangles.x > TO_DEG16(89))
    plr->viewangles.x = TO_DEG16(89);

  ped->v.angles.x = 0;
  ped->v.angles.y = plr->viewangles.y;
  ped->v.angles.z = 0;

  PM_PlayerMove(dt);
}
