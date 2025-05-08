#include <string.h>
#include "common.h"
#include "world.h"
#include "render.h"
#include "entity.h"
#include "game.h"
#include "progs.h"
#include "sound.h"
#include "menu.h"
#include "screen.h"
#include "move.h"

game_state_t gs;

static char g_nextmap[MAX_OSPATH];

void G_ParseMapEnts(bmodel_t *mdl) {
  // worldspawn
  gs.edicts[0].free = false;
  gs.edicts[0].v.solid = SOLID_BSP;
  gs.edicts[0].v.model = gs.worldmodel;
  gs.edicts[0].v.mins = gs.worldmodel->mins;
  gs.edicts[0].v.maxs = gs.worldmodel->maxs;

  // player 0
  gs.edicts[1].free = false;
  gs.edicts[1].v.classname = mdl->mapents[1].classname;
  gs.edicts[1].v.origin = mdl->mapents[1].origin;
  gs.edicts[1].v.angles = mdl->mapents[1].angles;

  // the rest
  edict_t *ent = &gs.edicts[2];
  xbspent_t *mapent = &mdl->mapents[2];
  int i;
  for (i = 2; i < mdl->nummapents; ++i, ++mapent) {
    if ((mapent->spawnflags & SPAWNFLAG_SKILL_MASK) == SPAWNFLAG_SKILL_MASK)
      continue; // deathmatch only
    ent->free = false;
    ent->v.classname = mapent->classname;
    ent->v.origin = mapent->origin;
    ent->v.angles = mapent->angles;
    ent->v.modelnum = mapent->model;
    ent->v.spawnflags = mapent->spawnflags;
    ent->v.noise = mapent->noise;
    ent->v.count = mapent->count;
    ent->v.health = mapent->health;
    ent->v.target = mapent->target;
    ent->v.killtarget = mapent->killtarget;
    ent->v.targetname = mapent->targetname;
    ent->v.extra_trigger.wait = mapent->wait;
    ent->v.extra_trigger.string = mapent->string;
    if (mapent->height)
      ent->v.velocity.z = TO_FIX32(mapent->height);
    ++ent;
  }

  gs.max_edict = ent - gs.edicts - 1;

  // mark the rest as free
  for (i = gs.max_edict + 1; i < gs.num_edicts; ++i, ++ent)
    ent->free = true;

  // this is no longer required
  Mem_Free(mdl->mapents);
  mdl->mapents = NULL;
}

void G_RequestMap(const char *mapname) {
  snprintf(g_nextmap, sizeof(g_nextmap), FS_BASE "\\MAPS\\%s.PSB;1", mapname);
}

qboolean G_CheckNextMap(void) {
  if (g_nextmap[0]) {
    G_StartMap(g_nextmap);
    g_nextmap[0] = 0;
  }
}

void G_StartMap(const char *path) {
  Mem_FreeToMark(MEM_MARK_LO);

  Scr_BeginLoading();

  memset(&gs, 0, sizeof(gs));

  Snd_NewMap();

  gs.worldmodel = NULL;

  gs.num_edicts = 512;
  gs.edicts = Mem_ZeroAlloc(gs.num_edicts * sizeof(edict_t));

  gs.worldmodel = Mod_LoadXBSP(path);
  gs.bmodels = gs.worldmodel->bmodelptrs;
  gs.amodels = gs.worldmodel->amodels;

  // clear area nodes
  G_ClearWorld();

  // spawn entities
  gs.max_edict = 1; // worldspawn, player
  G_ParseMapEnts(gs.worldmodel);

  Sys_Printf("psb loaded, %d entities, %d amodels, free mem: %u\n",
    gs.worldmodel->nummapents, gs.worldmodel->numamodels, Mem_GetFreeSpace());

  Sys_Printf("player start at (%d, %d, %d)\n",
    gs.edicts[1].v.origin.x >> FIXSHIFT,
    gs.edicts[1].v.origin.y >> FIXSHIFT,
    gs.edicts[1].v.origin.z >> FIXSHIFT);

  R_NewMap();

  Progs_NewMap();

  edict_t *ent = gs.edicts + 1;
  for (int i = 1; i <= gs.max_edict; ++i, ++ent) {
    G_SetModel(ent, ent->v.modelnum);
    // TODO: move this to an appropriate place
    // execute the spawn function
    ent_spawnfuncs[ent->v.classname](ent);
    G_LinkEdict(ent, false);
  }

  Scr_EndLoading();
}

void G_Update(const x16 dt) {
  x32vec3_t noclipvel;
  player_state_t *plr = &gs.player[0];
  edict_t *ped = plr->ent;

  if (menu_open) {
    Menu_Update();
    return;
  }

  plr->viewangles.y += XMUL16(plr->anglemove.y, dt);
  plr->viewangles.x += XMUL16(plr->anglemove.x, dt);
  if (plr->viewangles.x < TO_DEG16(-89))
    plr->viewangles.x = TO_DEG16(-89);
  else if (plr->viewangles.x > TO_DEG16(89))
    plr->viewangles.x = TO_DEG16(89);

  plr->punchangle += XMUL16(TO_DEG16(10), gs.frametime);
  if (plr->punchangle > 0)
    plr->punchangle = 0;

  ped->v.angles.x = 0;
  ped->v.angles.y = plr->viewangles.y;
  ped->v.angles.z = 0;

  G_Physics();
}

amodel_t *G_FindAliasModel(const s16 modelid) {
  for (int i = 0; i < gs.worldmodel->numamodels; ++i) {
    if (gs.amodels[i].id == modelid) {
      return &gs.amodels[i];
    }
  }
  Sys_Error("G_SetModel(0x%02x): not a valid amodel", modelid);
  return NULL;
}

void G_SetModel(edict_t *ent, s16 modelnum) {
  ent->v.modelnum = modelnum;

  if (modelnum < 0) {
    // for a brush model, modelnum is the negative of its index
    modelnum = -modelnum;
    if (modelnum >= gs.worldmodel->numsubmodels)
      Sys_Error("G_SetModel(%d): not a valid bmodel", -modelnum);
    const bmodel_t *bmodel = gs.bmodels[modelnum];
    ent->v.model = (void *)bmodel;
    G_SetSize(ent, &bmodel->mins, &bmodel->maxs);
  } else if (modelnum) {
    // for an alias model, modelnum is its positive unique id number
    ent->v.model = G_FindAliasModel(modelnum);
  } else {
    ent->v.model = NULL;
    ent->v.flags &= ~FL_VISIBLE;
  }
}

void G_SetSize(edict_t *ent, const x32vec3_t *mins, const x32vec3_t *maxs) {
  ent->v.mins = *mins;
  ent->v.maxs = *maxs;
  XVecSub(maxs, mins, &ent->v.size);
}
