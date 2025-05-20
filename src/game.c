#include <string.h>
#include "common.h"
#include "world.h"
#include "render.h"
#include "entity.h"
#include "system.h"
#include "progs.h"
#include "sound.h"
#include "menu.h"
#include "screen.h"
#include "input.h"
#include "move.h"
#include "cd.h"
#include "game.h"

game_state_t gs;

static char g_nextmap[MAX_OSPATH];

void G_ParseMapEnts(bmodel_t *mdl) {
  // worldspawn
  gs.edicts[0].free = false;
  gs.edicts[0].v.solid = SOLID_BSP;
  gs.edicts[0].v.model = gs.worldmodel;
  gs.edicts[0].v.mins = gs.worldmodel->mins;
  gs.edicts[0].v.maxs = gs.worldmodel->maxs;
  gs.edicts[0].v.noise = mdl->mapents[0].noise; // CD track

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
    if ((mapent->spawnflags & SPAWNFLAG_NOT_EASY) && gs.skill == 0)
      continue; // we're playing on easy and this does not spawn on easy
    if ((mapent->spawnflags & SPAWNFLAG_NOT_MEDIUM) && gs.skill == 1)
      continue; // we're playing on normal and this does not spawn on normal
    if ((mapent->spawnflags & SPAWNFLAG_NOT_HARD) && gs.skill >= 2)
      continue; // we're playing on hard or nightmare and this does not spawn on hard
    ent->free = false;
    ent->v.classname = mapent->classname;
    ent->v.origin = mapent->origin;
    ent->v.angles = mapent->angles;
    ent->v.modelnum = mapent->model;
    ent->v.spawnflags = mapent->spawnflags;
    ent->v.noise = mapent->noise;
    ent->v.count = mapent->count;
    ent->v.speed = mapent->speed;
    ent->v.dmg = mapent->dmg;
    ent->v.health = mapent->health;
    ent->v.target = mapent->target;
    ent->v.killtarget = mapent->killtarget;
    ent->v.targetname = mapent->targetname;
    ent->v.delay = mapent->delay;
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
    return true;
  }
  return false;
}

void G_StartMap(const char *path) {
  // disable CDDA but keep the motor spinning because we need to read
  CD_SetMode(CDMODE_DATA);

  Mem_FreeToMark(MEM_MARK_LO);

  Scr_BeginLoading();

  // wipe game state, but preserve skill and player inventory
  const stats_t stats = gs.player[0].stats;
  const s16 skill = gs.skill;
  memset(&gs, 0, sizeof(gs));
  gs.skill = skill;
  gs.player[0].stats = stats;

  Snd_NewMap();

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

  Sys_Printf("skill is %d\n", gs.skill);

  R_NewMap();

  Progs_NewMap();

  const s16 max_edict = gs.max_edict; // don't spawn entities created during the loop
  edict_t *ent = gs.edicts + 1;
  for (int i = 1; i <= max_edict; ++i, ++ent) {
    G_SetModel(ent, ent->v.modelnum);
    // TODO: move this to an appropriate place
    // execute the spawn function
    ent->v.nextthink = -1;
    if (ent_spawnfuncs[ent->v.classname])
      ent_spawnfuncs[ent->v.classname](ent);
    G_LinkEdict(ent, false);
  }

  Sys_Printf("after spawn: %d entities, free mem: %u\n",
    gs.max_edict + 1, Mem_GetFreeSpace());

  // if there is an audio track set for this map, play it
  // otherwise just stop the drive motor
  if (gs.edicts[0].v.noise)
    CD_PlayAudio(gs.edicts[0].v.noise);
  else
    CD_Stop();

  Scr_EndLoading();
}

static inline void ToggleFlag(player_state_t *plr, const u16 btn, const u32 flag) {
  if (IN_ButtonPressed(btn))
    plr->buttons ^= flag;
}

static inline void PressFlag(player_state_t *plr, const u16 btn, const u32 flag) {
  if (IN_ButtonPressed(btn))
    plr->buttons |= flag;
  else
    plr->buttons &= ~flag;
}

static inline void HoldFlag(player_state_t *plr, const u16 btn, const u32 flag) {
  if (IN_ButtonHeld(btn))
    plr->buttons |= flag;
  else
    plr->buttons &= ~flag;
}

static void UpdatePlayerInput(player_state_t *plr, const x16 dt) {
  // pause input inhibits all
  if (IN_ButtonPressed(PAD_START)) {
    Menu_Toggle();
    return;
  }

  // assume the player is using an analog pad and maybe a mouse for now

  // move inputs

  s32 up = IN_ButtonHeld(PAD_CROSS | PAD_L2) - IN_ButtonHeld(PAD_R3 | PAD_L1);
  s32 fwd = IN_ButtonHeld(PAD_UP) -  IN_ButtonHeld(PAD_DOWN);
  s32 side = IN_ButtonHeld(PAD_RIGHT) -  IN_ButtonHeld(PAD_LEFT);
  if (!fwd && !side) {
    // if digital inputs are not held, map left stick to digital directions
    fwd = (in.sticks[0].y < -0x40) - (in.sticks[0].y > 0x40);
    side = (in.sticks[0].x > 0x40) - (in.sticks[0].x < -0x40);
  }

  // look inputs: use rstick and mouse

  const x16 pitch = in.sticks[1].y + XMUL16(TO_FIX32(3), in.mouse.y);
  const x16 yaw = in.sticks[1].x + XMUL16(TO_FIX32(3), in.mouse.x);

  // buttons

  if (IN_ButtonPressed(PAD_R1)) {
    if (plr->ent->v.movetype == MOVETYPE_WALK)
      plr->ent->v.movetype = MOVETYPE_NOCLIP;
    else
      plr->ent->v.movetype = MOVETYPE_WALK;
  }

  if (IN_ButtonPressed(PAD_SELECT))
    rs.debug = !rs.debug;

  if (plr->ent->v.movetype == MOVETYPE_NOCLIP)
    plr->buttons &= ~BTN_SPEED;
  else
    ToggleFlag(plr, PAD_L3, BTN_SPEED);
  HoldFlag(plr, PAD_R2, BTN_FIRE);
  HoldFlag(plr, PAD_CROSS, BTN_JUMP);
  PressFlag(plr, PAD_TRIANGLE, BTN_PREVWEAPON);
  PressFlag(plr, PAD_CIRCLE, BTN_NEXTWEAPON);

  // transform look/move inputs into direction vectors
  const int onspeed = (plr->buttons & BTN_SPEED) != 0;
  plr->move.x = fwd * G_FORWARDSPEED;
  plr->move.y = side * G_FORWARDSPEED;
  plr->move.z = up * G_FORWARDSPEED;
  plr->anglemove.x = XMUL16(TO_FIX32(8), pitch);
  plr->anglemove.y = XMUL16(TO_FIX32(8), -yaw);
  plr->movespeed = G_FORWARDSPEED << onspeed;
}

void G_Update(const x16 dt) {
  player_state_t *plr = &gs.player[0];
  edict_t *ped = plr->ent;

  UpdatePlayerInput(plr, dt);

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

  if (plr->punchangle) {
    plr->punchangle += XMUL16(TO_DEG16(10), gs.frametime);
    if (plr->punchangle > 0)
      plr->punchangle = 0;
  }

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
