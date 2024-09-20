#include "../common.h"
#include "../game.h"
#include "../entity.h"
#include "../system.h"
#include "entclasses.h"

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
