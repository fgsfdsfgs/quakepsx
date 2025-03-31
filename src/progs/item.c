#include "prcommon.h"

// quick hack to test sounds
static void item_touch(edict_t *self, edict_t *other) {
  if (!other || other->v.classname != ENT_PLAYER)
    return;

  s16 sfxid;
  switch (self->v.classname) {
    case ENT_WEAPON_GRENADELAUNCHER:
    case ENT_WEAPON_LIGHTNING:
    case ENT_WEAPON_NAILGUN:
    case ENT_WEAPON_ROCKETLAUNCHER:
    case ENT_WEAPON_SUPERNAILGUN:
    case ENT_WEAPON_SUPERSHOTGUN:
      sfxid = SFXID_WEAPONS_PKUP;
      break;

    case ENT_ITEM_ARMOR1:
    case ENT_ITEM_ARMOR2:
    case ENT_ITEM_ARMORINV:
      sfxid = SFXID_ITEMS_ARMOR1;
      break;

    case ENT_ITEM_KEY1:
    case ENT_ITEM_KEY2:
      sfxid = SFXID_MISC_MEDKEY;
      break;

    case ENT_ITEM_HEALTH:
      sfxid = SFXID_ITEMS_HEALTH1;
      break;

    default:
      sfxid = SFXID_WEAPONS_LOCK4;
      break;
  }

  Snd_StartSoundId(other - gs.edicts, CHAN_ITEM, sfxid, &other->v.origin, SND_MAXVOL, ATTN_NORM);

  G_SetModel(self, 0);
  self->v.solid = SOLID_NOT;
}

void spawn_item(edict_t *self, s16 model) {
  if (model)
    G_SetModel(self, model);
  XVecSetInt(&self->v.mins, -16, -16, +0);
  XVecSetInt(&self->v.maxs, +16, +16, +56);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.solid = SOLID_TRIGGER;
  self->v.flags = FL_ITEM;
  self->v.touch = item_touch;
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

void spawn_item_health(edict_t *self) {
  spawn_item(self, 0);
}

void spawn_item_shells(edict_t *self) {
  spawn_item(self, 0);
}

void spawn_item_cells(edict_t *self) {
  spawn_item(self, 0);
}

void spawn_item_rockets(edict_t *self) {
  spawn_item(self, 0);
}

void spawn_item_spikes(edict_t *self) {
  spawn_item(self, 0);
}
