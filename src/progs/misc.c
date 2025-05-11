#include "prcommon.h"

#define SF_LIGHT_START_OFF 1

static inline void spawn_ambient(edict_t *self, const s16 sfxid, const s16 vol) {
  Snd_StaticSoundId(sfxid, &self->v.origin, vol, ATTN_STATIC);
}

void spawn_ambient_comp_hum(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_COMP1, SND_MAXVOL);
}

void spawn_ambient_drip(edict_t *self) {
  // spawn_ambient(self, SFXID_AMBIENCE_DRIP1, SND_MAXVOL >> 1);
}

void spawn_ambient_drone(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_DRONE6, SND_MAXVOL >> 1);
}

void spawn_ambient_suck_wind(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_SUCK1, SND_MAXVOL);
}

void spawn_ambient_swamp1(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_SWAMP1, SND_MAXVOL >> 1);
}

void spawn_ambient_swamp2(edict_t *self) {
  // SWAMP2 is basically the same as SWAMP1
  spawn_ambient(self, SFXID_AMBIENCE_SWAMP1, SND_MAXVOL >> 1);
}

static void explobox_explode(edict_t *self, edict_t *killer) {
  self->v.flags &= ~(FL_TAKEDAMAGE | FL_AUTOAIM);
  utl_sound(self, CHAN_VOICE, SFXID_WEAPONS_R_EXP3, SND_MAXVOL, ATTN_NORM);
  utl_radius_damage(self, self, 160, self);
  self->v.origin.z += TO_FIX32(32);
  utl_become_explosion(self);
}

void spawn_misc_explobox(edict_t *self) {
  G_SetModel(self, MDLID_B_EXPLOB);
  self->v.mins = ((amodel_t *)self->v.model)->mins;
  self->v.maxs = ((amodel_t *)self->v.model)->maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);

  self->v.solid = SOLID_BBOX;
  self->v.movetype = MOVETYPE_NONE;
  self->v.health = 20;
  self->v.th_die = explobox_explode;
  self->v.flags |= FL_TAKEDAMAGE | FL_AUTOAIM;

  self->v.origin.z += TO_FIX32(2);
  G_DropToFloor(self);
}

void spawn_misc_explobox2(edict_t *self) {
  G_SetModel(self, MDLID_B_EXBOX2);
  self->v.mins = ((amodel_t *)self->v.model)->mins;
  self->v.maxs = ((amodel_t *)self->v.model)->maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);

  self->v.solid = SOLID_BBOX;
  self->v.movetype = MOVETYPE_NONE;
  self->v.health = 20;
  self->v.th_die = explobox_explode;
  self->v.flags |= FL_TAKEDAMAGE | FL_AUTOAIM;

  self->v.origin.z += TO_FIX32(2);
  G_DropToFloor(self);
}

static void light_use(edict_t *self, edict_t *activator) {
  if (self->v.spawnflags & SF_LIGHT_START_OFF) {
    R_SetLightStyle(self->v.count, "m");
    self->v.spawnflags &= ~SF_LIGHT_START_OFF;
  } else {
    R_SetLightStyle(self->v.count, "a");
    self->v.spawnflags |= SF_LIGHT_START_OFF;
  }
}

void spawn_light(edict_t *self) {
  if (!self->v.targetname) {
    // inert light
    utl_remove_delayed(self);
    return;
  }

  // style
  if (self->v.count >= 32) {
    self->v.use = light_use;
    if (self->v.spawnflags & SF_LIGHT_START_OFF)
      R_SetLightStyle(self->v.count, "a");
    else
      R_SetLightStyle(self->v.count, "m");
  }
}

void spawn_light_fluoro(edict_t *self) {
  spawn_light(self);
  // TODO: hum?
}

void spawn_light_fluorospark(edict_t *self) {
  if (!self->v.count)
    self->v.count = 10;
  // TODO: buzz?
}

void spawn_light_globe(edict_t *self) {
  /* unused */
}

void spawn_light_flame_large_yellow(edict_t *self) {
  // TODO
}

void spawn_light_flame_small_white(edict_t *self) {
  // TODO
}

void spawn_light_flame_small_yellow(edict_t *self) {
  // TODO
}

void spawn_light_torch_small_walltorch(edict_t *self) {
  // TODO
}
