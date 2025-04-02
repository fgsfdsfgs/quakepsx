#include "prcommon.h"

static void monster_demon1_think(edict_t *self) {
  cycler_think(self, 0, 8);
}

void spawn_monster_demon1(edict_t *self) {
  G_SetModel(self, MDLID_DEMON);
  self->v.mins = gs.worldmodel->hulls[2].mins;
  self->v.maxs = gs.worldmodel->hulls[2].maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_demon1_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
