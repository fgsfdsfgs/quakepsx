#include "prcommon.h"

void spawn_func_button(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_illusionary(edict_t *self) {
  self->v.solid = SOLID_NOT;
}

static void func_wall_use(edict_t *self, edict_t *activator) {
  // change to alternate textures
  self->v.frame = 1 - self->v.frame;
}

void spawn_func_wall(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.use = func_wall_use;
}

void spawn_func_bossgate(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_episodegate(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  // TODO
  self->v.nextthink = gs.time + PR_FRAMETIME;
  self->v.think = utl_remove;
}
