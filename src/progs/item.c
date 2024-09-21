#include "prcommon.h"


void spawn_item(edict_t *self, s16 model) {
  G_SetModel(self, model);
  XVecSetInt(&self->v.mins, -16, -16, +0);
  XVecSetInt(&self->v.maxs, +16, +16, +56);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.solid = SOLID_TRIGGER;
  self->v.flags = FL_ITEM;
}

void spawn_item_armor1(edict_t *self) {
  spawn_item(self, MDLID_ARMOR);
}

void spawn_item_armor2(edict_t *self) {
  spawn_item(self, MDLID_ARMOR);
}

void spawn_item_armorInv(edict_t *self) {
  spawn_item(self, MDLID_ARMOR);
}

void spawn_item_artifact_envirosuit(edict_t *self) {
  spawn_item(self, MDLID_SUIT);
}

void spawn_item_artifact_invisibility(edict_t *self) {
  spawn_item(self, MDLID_INVISIBL);
}

void spawn_item_artifact_invulnerability(edict_t *self) {
  spawn_item(self, MDLID_INVULNER);
}

void spawn_item_artifact_super_damage(edict_t *self) {
  spawn_item(self, MDLID_QUADDAMA);
}

void spawn_item_weapon(edict_t *self) {
  // what the fuck is this
}

void spawn_weapon_grenadelauncher(edict_t *self) {
  spawn_item(self, MDLID_G_ROCK);
}

void spawn_weapon_lightning(edict_t *self) {
  spawn_item(self, MDLID_G_LIGHT);
}

void spawn_weapon_nailgun(edict_t *self) {
  spawn_item(self, MDLID_G_NAIL);
}

void spawn_weapon_rocketlauncher(edict_t *self) {
  spawn_item(self, MDLID_G_ROCK2);
}

void spawn_weapon_supernailgun(edict_t *self) {
  spawn_item(self, MDLID_G_NAIL2);
}

void spawn_weapon_supershotgun(edict_t *self) {
  spawn_item(self, MDLID_G_SHOT);
}
