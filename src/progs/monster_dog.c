#include "prcommon.h"
#include "monster.h"

void spawn_monster_dog(edict_t *self) {
  G_SetModel(self, MDLID_DOG);
  XVecSetInt(&self->v.mins, -32, -32, -24);
  XVecSetInt(&self->v.maxs, +32, +32, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = cycler_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
