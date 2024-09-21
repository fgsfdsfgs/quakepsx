#include "../common.h"
#include "../game.h"
#include "../entity.h"
#include "../system.h"
#include "entclasses.h"

static void dog_think(edict_t *self) {
  self->v.frame++;
  if (self->v.frame >= ((amodel_t *)self->v.model)->numframes)
    self->v.frame = 0;
  self->v.nextthink = gs.time + 41;
}

void spawn_monster_dog(edict_t *self) {
  G_SetModel(self, 0x08 /*dog.mdl*/);
  self->v.mins.x = TO_FIX32(-32);
  self->v.mins.y = TO_FIX32(-32);
  self->v.mins.z = TO_FIX32(-24);
  self->v.maxs.x = TO_FIX32(+32);
  self->v.maxs.y = TO_FIX32(+32);
  self->v.maxs.z = TO_FIX32(+40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = dog_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
