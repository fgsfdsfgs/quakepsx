#include "prcommon.h"

void spawn_func_bossgate(edict_t *self) {

}

void spawn_func_button(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_door(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_door_secret(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_episodegate(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_illusionary(edict_t *self) {

}

void spawn_func_plat(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_train(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_func_wall(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}

void spawn_misc_explobox(edict_t *self) {
  self->v.mins = ((amodel_t *)self->v.model)->mins;
  self->v.maxs = ((amodel_t *)self->v.model)->maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.solid = SOLID_BBOX;
  self->v.movetype = MOVETYPE_NONE;
}

void spawn_misc_explobox2(edict_t *self) {
  self->v.mins = ((amodel_t *)self->v.model)->mins;
  self->v.maxs = ((amodel_t *)self->v.model)->maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.solid = SOLID_BBOX;
  self->v.movetype = MOVETYPE_NONE;
}
