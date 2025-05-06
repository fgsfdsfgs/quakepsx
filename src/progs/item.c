#include "prcommon.h"

#define SPAWNFLAG_AMMO_BIG 1
#define SPAWNFLAG_HEALTH_SMALL 1
#define SPAWNFLAG_HEALTH_SUPER 2

static const char *ammo_names[] = { "shells", "nails", "rockets", "cells" };
static const s16 ammo_max[] = { 100, 200, 100, 100 };

// quick hack to test sounds
static void item_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  if (self->v.noise)
    Snd_StartSoundId(other - gs.edicts, CHAN_ITEM, self->v.noise, &other->v.origin, SND_MAXVOL, ATTN_NORM);
  Scr_SetBlend(C_YELLOW, SCR_FLASH_TIME);

  G_SetModel(self, 0);
  self->v.solid = SOLID_NOT;

  ED_Free(self);
}

static void ammo_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  player_state_t *plr = other->v.player;
  const s16 ammo_type = (s16)self->v.extra_item.ammotype;
  const s16 ammo_count = (s16)self->v.count;
  if (plr->stats.ammo[ammo_type] >= ammo_max[ammo_type])
    return;

  plr->stats.ammo[ammo_type] += ammo_count;
  if (plr->stats.ammo[ammo_type] > ammo_max[ammo_type])
    plr->stats.ammo[ammo_type] = ammo_max[ammo_type];

  Scr_SetTopMsg(VA("You get %d %s", ammo_count, ammo_names[ammo_type]));

  item_touch(self, other);
}

static void weapon_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  player_state_t *plr = other->v.player;

  s16 ammo = plr->stats.ammo[self->v.extra_item.ammotype];
  const s16 max_ammo = ammo_max[self->v.extra_item.ammotype];

  if (plr->stats.items & self->v.extra_item.type) {
    if (ammo >= max_ammo)
      return;
  }

  s16 index;
  switch (self->v.extra_item.type) {
  case IT_SUPER_SHOTGUN:    index = WEAP_SUPER_SHOTGUN; break;
  case IT_NAILGUN:          index = WEAP_NAILGUN; break;
  case IT_SUPER_NAILGUN:    index = WEAP_SUPER_NAILGUN; break;
  case IT_GRENADE_LAUNCHER: index = WEAP_GRENADE_LAUNCHER; break;
  case IT_ROCKET_LAUNCHER:  index = WEAP_ROCKET_LAUNCHER; break;
  case IT_LIGHTNING:        index = WEAP_LIGHTNING; break;
  default: return;
  }

  plr->stats.items |= self->v.extra_item.type;
  ammo += self->v.count;
  if (ammo > max_ammo)
    ammo = max_ammo;
  plr->stats.ammo[self->v.extra_item.ammotype] = ammo;

  Scr_SetTopMsg(VA("You got the %s!", weap_table[index].name));

  if (plr->stats.weaponnum != index)
    Player_SetWeapon(other, index);

  item_touch(self, other);
}

static void health_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  player_state_t *plr = other->v.player;

  if (self->v.extra_item.type == 2) {
    // megahealth
    if (other->v.health >= 250)
      return;
    // if (!T_Heal(other, self->v.count, true))
    //   return;
    plr->stats.items |= IT_SUPERHEALTH;
  } else {
    if (other->v.health >= other->v.max_health)
      return;
    // if (!T_Heal(other, self->v.count, false))
    //   return;
  }

  Scr_SetTopMsg(VA("You receive %d health", self->v.count));

  item_touch(self, other);
}

static void armor_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  player_state_t *plr = other->v.player;
  if (plr->stats.armor >= self->v.count)
    return;

  plr->stats.armor += self->v.count;
  if (plr->stats.armor > self->v.count)
    plr->stats.armor = self->v.count;

  Scr_SetTopMsg("You got armor");

  item_touch(self, other);
}

static void place_item(edict_t *self) {
  XVecSetInt(&self->v.mins, +0, +0, +0);
  XVecSetInt(&self->v.maxs, +32, +32, +56);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.solid = SOLID_TRIGGER;
  self->v.flags = FL_ITEM;
  self->v.movetype = MOVETYPE_TOSS;
  self->v.velocity = x32vec3_origin;
  self->v.origin.z += TO_FIX32(6);

  if (!G_DropToFloor(self)) {
    Sys_Printf(
      "Item %d (%02x) fell out of world at (%d, %d, %d)\n",
      EDICT_NUM(self), self->v.classname,
      self->v.origin.x >> FIXSHIFT, self->v.origin.y >> FIXSHIFT, self->v.origin.z >> FIXSHIFT
    );
    ED_Free(self);
  }
}

static void spawn_item(edict_t *self, const s16 model, const u32 type, const s16 count, const u8 noise) {
  if (model)
    G_SetModel(self, model);
  self->v.think = place_item;
  self->v.touch = item_touch;
  self->v.nextthink = gs.time + PR_FRAMETIME * 2;
  self->v.extra_item.type = type;
  self->v.count = count;
  self->v.noise = noise;
}

static void spawn_ammo(edict_t *self, const s16 mdl0, const s16 mdl1, const s16 num0, const s16 num1, const u16 ammo_type) {
  if (self->v.spawnflags & SPAWNFLAG_AMMO_BIG)
    spawn_item(self, mdl1, 0, num1, SFXID_WEAPONS_LOCK4);
  else
    spawn_item(self, mdl0, 0, num0, SFXID_WEAPONS_LOCK4);
  self->v.extra_item.ammotype = ammo_type;
  self->v.touch = ammo_touch;
}

static void spawn_weapon(edict_t *self, const s16 model, const u32 type, const u16 ammo_type, const s16 ammo_count) {
  spawn_item(self, model, type, ammo_count, SFXID_WEAPONS_PKUP);
  self->v.extra_item.ammotype = ammo_type;
  self->v.touch = weapon_touch;
}

void spawn_item_armor1(edict_t *self) {
  spawn_item(self, MDLID_ARMOR, IT_ARMOR1, 100, SFXID_ITEMS_ARMOR1);
  self->v.touch = armor_touch;
}

void spawn_item_armor2(edict_t *self) {
  spawn_item(self, MDLID_ARMOR, IT_ARMOR2, 150, SFXID_ITEMS_ARMOR1);
  self->v.touch = armor_touch;
}

void spawn_item_armorInv(edict_t *self) {
  spawn_item(self, MDLID_ARMOR, IT_ARMOR3, 200, SFXID_ITEMS_ARMOR1);
  self->v.touch = armor_touch;
}

void spawn_item_artifact_envirosuit(edict_t *self) {
  spawn_item(self, MDLID_SUIT, IT_SUIT, 0, SFXID_ITEMS_SUIT);
}

void spawn_item_artifact_invisibility(edict_t *self) {
  spawn_item(self, MDLID_INVISIBL, IT_INVISIBILITY, 0, SFXID_ITEMS_INV1);
}

void spawn_item_artifact_invulnerability(edict_t *self) {
  spawn_item(self, MDLID_INVULNER, IT_INVULNERABILITY, 0, SFXID_ITEMS_PROTECT);
}

void spawn_item_artifact_super_damage(edict_t *self) {
  spawn_item(self, MDLID_QUADDAMA, IT_QUAD, 0, SFXID_ITEMS_DAMAGE);
}

void spawn_item_weapon(edict_t *self) {
  // what the fuck is this
}

void spawn_weapon_grenadelauncher(edict_t *self) {
  spawn_weapon(self, MDLID_G_ROCK, IT_GRENADE_LAUNCHER, AMMO_ROCKETS, 5);
}

void spawn_weapon_lightning(edict_t *self) {
  spawn_weapon(self, MDLID_G_LIGHT, IT_LIGHTNING, AMMO_CELLS, 15);
}

void spawn_weapon_nailgun(edict_t *self) {
  spawn_weapon(self, MDLID_G_NAIL, IT_NAILGUN, AMMO_NAILS, 30);
}

void spawn_weapon_rocketlauncher(edict_t *self) {
  spawn_weapon(self, MDLID_G_ROCK2, IT_ROCKET_LAUNCHER, AMMO_ROCKETS, 5);
}

void spawn_weapon_supernailgun(edict_t *self) {
  spawn_weapon(self, MDLID_G_NAIL2, IT_SUPER_NAILGUN, AMMO_NAILS, 30);
}

void spawn_weapon_supershotgun(edict_t *self) {
  spawn_weapon(self, MDLID_G_SHOT, IT_SUPER_SHOTGUN, AMMO_ROCKETS, 5);
}

void spawn_item_health(edict_t *self) {
  if (self->v.spawnflags & SPAWNFLAG_HEALTH_SMALL) {
    spawn_item(self, MDLID_B_BH10, 0, 15, SFXID_ITEMS_R_ITEM1);
  } else if (self->v.spawnflags & SPAWNFLAG_HEALTH_SUPER) {
    spawn_item(self, MDLID_B_BH100, 2, 100, SFXID_ITEMS_R_ITEM2);
  } else {
    spawn_item(self, MDLID_B_BH25, 1, 25, SFXID_ITEMS_HEALTH1);
  }
  self->v.touch = health_touch;
}

void spawn_item_shells(edict_t *self) {
  spawn_ammo(self, MDLID_B_SHELL0, MDLID_B_SHELL1, 20, 40, AMMO_SHELLS);
}

void spawn_item_cells(edict_t *self) {
  spawn_ammo(self, MDLID_B_BATT0, MDLID_B_BATT1, 6, 12, AMMO_CELLS);
}

void spawn_item_rockets(edict_t *self) {
  spawn_ammo(self, MDLID_B_ROCK0, MDLID_B_ROCK1, 5, 10, AMMO_ROCKETS);
}

void spawn_item_spikes(edict_t *self) {
  spawn_ammo(self, MDLID_B_NAIL0, MDLID_B_NAIL1, 25, 50, AMMO_NAILS);
}
