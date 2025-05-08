#include "prcommon.h"

void fx_spawn_blood(const x32vec3_t *org, const s16 damage) {
  R_SpawnParticleEffect(PT_GRAV, org, &x32vec3_origin, 73, 1 + (damage >> 1));
}

void fx_spawn_gunshot(const x32vec3_t *org) {
  R_SpawnParticleEffect(PT_GRAV, org, &x32vec3_origin, 1, 3);
}

void fx_spawn_explosion(const x32vec3_t *org) {
  R_SpawnParticleExplosion(org);
}

static void telefog_play(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_MISC_R_TELE3, SND_MAXVOL, ATTN_NORM);
  utl_remove(self);
}

void fx_spawn_telefog(const x32vec3_t *org) {
  edict_t *s = ED_Alloc();
  s->v.classname = ENT_TEMP_ENTITY;
  s->v.origin = *org;
  s->v.nextthink = gs.time + PR_FRAMETIME * 2;
  s->v.think = telefog_play;
  R_SpawnParticleTeleport(org);
}

static inline void velocity_for_damage(x32vec3_t *vel, const s16 damage) {
  vel->x = 200 * xrand32();
  vel->y = 200 * xrand32();
  vel->z = 200 + 200 * xrand32();
  if (damage < -50 && damage > -200) {
    vel->x <<= 1;
    vel->y <<= 1;
    vel->z <<= 1;
  } else if (damage <= -200) {
    vel->x <<= 2;
    vel->y <<= 2;
    vel->z <<= 2;
  }
}

void fx_throw_gib(const x32vec3_t *org, const s16 mdlid, const s16 damage) {
  edict_t *gib = ED_Alloc();
  gib->v.classname = ENT_TEMP_ENTITY;
  gib->v.origin = *org;
  gib->v.origin.z += TO_FIX32(1);
  gib->v.solid = SOLID_NOT;
  gib->v.movetype = MOVETYPE_BOUNCE;
  gib->v.ltime = gs.time;
  gib->v.effects = EF_GIB;
  velocity_for_damage(&gib->v.velocity, damage);
  gib->v.avelocity.x = xrand32();
  gib->v.avelocity.y = xrand32();
  gib->v.avelocity.z = xrand32();
  gib->v.nextthink = gs.time + TO_FIX32(10) + 5 * xrand32();
  gib->v.think = utl_remove;
  G_SetModel(gib, mdlid);
  G_SetSize(gib, &gib->v.mins, &gib->v.maxs);
  G_LinkEdict(gib, false);
}

void fx_throw_head(edict_t *self, const s16 mdlid, const s16 damage) {
  self->v.frame = 0;
  self->v.think = null_think;
  self->v.nextthink = -1;
  self->v.movetype = MOVETYPE_BOUNCE;
  self->v.solid = SOLID_NOT;
  self->v.flags &= ~(FL_TAKEDAMAGE | FL_AUTOAIM | FL_ONGROUND);
  self->v.viewheight = TO_FIX32(8);
  self->v.ltime = gs.time;
  self->v.effects = EF_GIB;
  velocity_for_damage(&self->v.velocity, damage);
  self->v.avelocity.y = xrand32();
  XVecSetInt(&self->v.mins, -1, -1, -1);
  XVecSetInt(&self->v.maxs, +1, +1, +1);
  G_SetModel(self, mdlid);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.origin.z += TO_FIX32(24);
  G_LinkEdict(self, false);
}
