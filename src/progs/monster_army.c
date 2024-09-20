#include "../common.h"
#include "../game.h"
#include "../entity.h"
#include "../system.h"
#include "entclasses.h"

static void army_think(edict_t *self) {
  self->v.frame++;
  if (self->v.frame >= ((amodel_t *)self->v.model)->numframes)
    self->v.frame = 0;
  self->v.nextthink = gs.time + 410;
}

void spawn_monster_army(edict_t *self) {
  G_SetModel(self, 0x22 /*soldier.mdl*/);
  self->v.mins.x = TO_FIX32(-16);
  self->v.mins.y = TO_FIX32(-16);
  self->v.mins.z = TO_FIX32(-24);
  self->v.maxs.x = TO_FIX32(+16);
  self->v.maxs.y = TO_FIX32(+16);
  self->v.maxs.z = TO_FIX32(+24);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = army_think;
  self->v.nextthink = gs.time + 410;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.flags = FL_MONSTER;
}
