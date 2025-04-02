#include "prcommon.h"

static void monster_fish_think(edict_t *self) {
  cycler_think(self, 40, 57);
}

void spawn_monster_fish(edict_t *self) {
  G_SetModel(self, MDLID_FISH);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +24);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_fish_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
