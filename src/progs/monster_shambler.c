#include "prcommon.h"

static void monster_shambler_think(edict_t *self) {
  cycler_think(self, 0, 16);
}

void spawn_monster_shambler(edict_t *self) {
  G_SetModel(self, MDLID_SHAMBLER);
  self->v.mins = gs.worldmodel->hulls[2].mins;
  self->v.maxs = gs.worldmodel->hulls[2].maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_shambler_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
