#include "prcommon.h"

// trigger_multiple
#define SPAWNFLAG_NOTOUCH 1

// all triggers
#define SPAWNFLAG_NOMESSAGE 1

// trigger_teleport
#define SPAWNFLAG_PLAYER_ONLY 1
#define SPAWNFLAG_SILENT 2

// trigger_push
#define SPAWNFLAG_PUSH_ONCE 1

// trigger_changelevel
#define SPAWNFLAG_NO_INTERMISSION 1

static void trigger_init(edict_t *self) {
  // trigger angles are used for one-way touches.  An angle of 0 is assumed
  // to mean no restrictions, so use a yaw of 360 instead.
  // We'll store the movedir in avelocity because it's unused by triggers.
  if (self->v.angles.x || self->v.angles.y || self->v.angles.z)
    utl_set_movedir(self, &self->v.avelocity);
  self->v.solid = SOLID_TRIGGER;
  self->v.movetype = MOVETYPE_NONE;
  // model/size should already be set by spawn dispatch
  self->v.model = NULL;
  self->v.modelnum = 0;
}

static void multi_wait(edict_t *self) {
  if (self->v.max_health) {
    self->v.health = self->v.max_health;
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.solid = SOLID_BBOX;
  }
}

static void multi_trigger(edict_t *self, edict_t *activator) {
  if (self->v.nextthink > gs.time)
    return; // already triggered

  if (self->v.classname == ENT_TRIGGER_SECRET) {
    if (activator->v.classname != ENT_PLAYER)
      return;
    pr.found_secrets++;
  }

  if (self->v.noise)
    utl_sound(self, CHAN_VOICE, self->v.noise, SND_MAXVOL, ATTN_NORM);

  // don't trigger again until reset
  self->v.flags &= ~FL_TAKEDAMAGE;

  utl_usetargets(self, activator);

  if (self->v.extra_trigger.wait > 0) {
    self->v.think = multi_wait;
    self->v.nextthink = gs.time + self->v.extra_trigger.wait;
  } else {
    // we can't just remove(self) here, because this is a touch function
    // called while engine code is looping through area links...
    self->v.touch = NULL;
    self->v.nextthink = gs.time + 1;
    self->v.think = utl_remove;
  }
}

static void multi_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  // if the trigger has an angles field, check player's facing direction
  if (self->v.avelocity.x || self->v.avelocity.y || self->v.avelocity.z) {
    utl_makevectors(&other->v.angles);
    if (XVecDotSS(&pr.v_forward, &self->v.avelocity) < 0)
      return; // not facing the right way
  }

  multi_trigger(self, other);
}

void spawn_trigger_multiple(edict_t *self) {
  switch (self->v.noise) {
  case 1:  self->v.noise = SFXID_MISC_SECRET; break;
  case 2:  self->v.noise = SFXID_MISC_TALK; break;
  case 3:  self->v.noise = SFXID_MISC_TRIGGER1; break;
  default: self->v.noise = 0; break;
  }

  if (self->v.extra_trigger.wait == 0)
    self->v.extra_trigger.wait = FTOX(0.2);

  self->v.use = multi_trigger;

  trigger_init(self);

  if (self->v.health) {
    self->v.max_health = self->v.health;
    self->v.th_die = multi_trigger;
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.solid = SOLID_BBOX;
  } else if ((self->v.spawnflags & SPAWNFLAG_NOTOUCH) == 0) {
    self->v.touch = multi_touch;
  }
}

void spawn_trigger_once(edict_t *self) {
  self->v.extra_trigger.wait = -1;
  spawn_trigger_multiple(self);
}

void spawn_trigger_relay(edict_t *self) {
  self->v.use = utl_usetargets;
}

void spawn_trigger_secret(edict_t *self) {
  pr.total_secrets++;
  self->v.extra_trigger.wait = -1;
  self->v.noise = 1;
  spawn_trigger_multiple(self);
}

static void counter_use(edict_t *self, edict_t *activator) {
  self->v.count--;
  if (self->v.count < 0)
    return;

  if (self->v.count > 0) {
    if (activator->v.classname == ENT_PLAYER && ((self->v.spawnflags & SPAWNFLAG_NOMESSAGE) == 0)) {
      if (self->v.count >= 4)
        Scr_SetCenterMsg("There are more to go...");
      else
        Scr_SetCenterMsg(VA("Only %d more to go...", self->v.count));
    }
    return;
  }

  if (activator->v.classname == ENT_PLAYER && ((self->v.spawnflags & SPAWNFLAG_NOMESSAGE) == 0))
    Scr_SetCenterMsg("Sequence completed!");

  multi_trigger(self, activator);
}

void spawn_trigger_counter(edict_t *self) {
  self->v.extra_trigger.wait = -1;
  if (!self->v.count)
    self->v.count = 2;
  self->v.use = counter_use;
}

static void teledeath_touch(edict_t *self, edict_t *other) {
  if (other == self->v.owner)
    return;

  // frag anyone who teleports in on top of an invincible player
  // other monsters explode themselves

  if (other->v.classname == ENT_PLAYER) {
    // TODO: invuln check
    if (self->v.owner && self->v.owner->v.classname != ENT_PLAYER) {
      utl_damage(self->v.owner, self, self, 30000);
      return;
    }
  }

  if (other->v.health)
    utl_damage(other, self, self, 30000);
}

static void spawn_teledeath(edict_t *owner, const x32vec3_t *org) {
  edict_t *death = ED_Alloc();
  death->v.classname = ENT_TEMP_ENTITY;
  death->v.solid = SOLID_TRIGGER;
  death->v.touch = teledeath_touch;
  death->v.think = utl_remove;
  death->v.nextthink = gs.time + PR_FRAMETIME * 2;
  death->v.origin = *org;
  death->v.owner = owner;
  death->v.mins.x = owner->v.mins.x - ONE;
  death->v.mins.y = owner->v.mins.y - ONE;
  death->v.mins.z = owner->v.mins.z - ONE;
  death->v.maxs.x = owner->v.maxs.x + ONE;
  death->v.maxs.y = owner->v.maxs.y + ONE;
  death->v.maxs.z = owner->v.maxs.z + ONE;
  gs.force_retouch = 2;
  G_SetSize(death, &death->v.mins, &death->v.maxs);
  G_LinkEdict(death, false);
}

static void teleport_touch(edict_t *self, edict_t *other) {
  if (self->v.targetname && self->v.nextthink < gs.time)
    return; // not fired yet

  if (self->v.spawnflags & SPAWNFLAG_PLAYER_ONLY) {
    if (other->v.classname != ENT_PLAYER)
      return;
  }

  // only teleport living creatures
  if (other->v.health <= 0 || other->v.solid != SOLID_SLIDEBOX)
    return;

  utl_usetargets(self, other);

  // put a tfog where the player was
  // ???

  edict_t *t = G_FindByTargetname(gs.edicts, self->v.target);
  if (!t || t == gs.edicts) {
    Sys_Printf("Teleporter %02x can't find target %x\n", EDICT_NUM(self), self->v.target);
    return;
  }

  utl_makevectors(&t->v.angles);

  // spawn a tfog flash in front of the destination
  x32vec3_t org;
  org.x = t->v.origin.x + pr.v_forward.x * 32;
  org.y = t->v.origin.y + pr.v_forward.y * 32;
  org.z = t->v.origin.z + pr.v_forward.z * 32;
  fx_spawn_telefog(&org);

  // kill anything at the destination point
  spawn_teledeath(other, &t->v.origin);

  if (!other->v.health) {
    other->v.origin = t->v.origin;
    other->v.velocity.x = xmul32(pr.v_forward.x, other->v.velocity.x) + xmul32(pr.v_forward.x, other->v.velocity.y);
    other->v.velocity.y = xmul32(pr.v_forward.y, other->v.velocity.x) + xmul32(pr.v_forward.y, other->v.velocity.y);
    other->v.velocity.z = xmul32(pr.v_forward.z, other->v.velocity.x) + xmul32(pr.v_forward.z, other->v.velocity.y);
    return;
  }

  // move the player and lock him down for a little while

  other->v.origin = t->v.origin;
  other->v.angles = t->v.angles;
  other->v.flags &= ~FL_ONGROUND;

  if (other->v.classname == ENT_PLAYER) {
    other->v.player->teleport_time = gs.time + FTOX(0.7);
    other->v.velocity.x = 300 * pr.v_forward.x;
    other->v.velocity.y = 300 * pr.v_forward.y;
    other->v.velocity.z = 300 * pr.v_forward.z;
    other->v.player->viewangles = t->v.angles;
  }
}

static void teleport_use(edict_t *self, edict_t *activator) {
  self->v.nextthink = gs.time + PR_FRAMETIME * 2;
  gs.force_retouch = 2; // make sure even still objects get hit
  self->v.think = NULL;
}

void spawn_trigger_teleport(edict_t *self) {
  trigger_init(self);

  self->v.touch = teleport_touch;
  self->v.use = teleport_use;

  if (!(self->v.spawnflags & SPAWNFLAG_SILENT)) {
    x32vec3_t org;
    org.x = (self->v.mins.x + self->v.maxs.x) >> 1;
    org.y = (self->v.mins.y + self->v.maxs.y) >> 1;
    org.z = (self->v.mins.z + self->v.maxs.z) >> 1;
    Snd_StaticSoundId(SFXID_AMBIENCE_HUM1, &org, SND_MAXVOL >> 1, ATTN_STATIC);
  }
}

void spawn_info_teleport_destination(edict_t *self) {
  // this does nothing, just serves as a target spot
  self->v.origin.z += TO_FIX32(27);
}

static void setskill_touch(edict_t *self, edict_t *other) {
  if (other->v.classname == ENT_PLAYER) {
    // TODO: skill
  }
}

void spawn_trigger_setskill(edict_t *self) {
  trigger_init(self);
  self->v.touch = setskill_touch;
}

static void onlyregistered_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  if (self->v.extra_trigger.wait > gs.time)
    return;

  self->v.extra_trigger.wait = gs.time + TO_FIX32(2);

  if (com_registered) {
    self->v.extra_trigger.string = 0;
    utl_usetargets(self, other);
    utl_remove(self);
    return;
  }

  if (self->v.extra_trigger.string) {
    utl_sound(other, CHAN_BODY, SFXID_MISC_TALK, SND_MAXVOL, ATTN_NORM);
    Scr_SetCenterMsg(gs.worldmodel->strings + self->v.extra_trigger.string);
  }
}

void spawn_trigger_onlyregistered(edict_t *self) {
  trigger_init(self);
  self->v.touch = onlyregistered_touch;
}

static void hurt_on(edict_t *self) {
  self->v.solid = SOLID_TRIGGER;
  self->v.nextthink = -1;
}

static void hurt_touch(edict_t *self, edict_t *other) {
  if (other->v.flags & FL_TAKEDAMAGE) {
    self->v.solid = SOLID_NOT;
    utl_damage(other, self, self, self->v.dmg);
    self->v.think = hurt_on;
    self->v.nextthink = gs.time + ONE;
  }
}

void spawn_trigger_hurt(edict_t *self) {
  trigger_init(self);
  self->v.touch = hurt_touch;
  if (!self->v.dmg)
    self->v.dmg = 5;
}

static void push_touch(edict_t *self, edict_t *other) {
  if (other->v.classname == ENT_GRENADE) {
    other->v.velocity.x = self->v.speed * 10 * self->v.avelocity.x;
    other->v.velocity.y = self->v.speed * 10 * self->v.avelocity.y;
    other->v.velocity.z = self->v.speed * 10 * self->v.avelocity.z;
  } else if (other->v.health > 0) {
    other->v.velocity.x = self->v.speed * 10 * self->v.avelocity.x;
    other->v.velocity.y = self->v.speed * 10 * self->v.avelocity.y;
    other->v.velocity.z = self->v.speed * 10 * self->v.avelocity.z;
    if (other->v.classname == ENT_PLAYER) {
      // TODO: fly sound
    }
  }
  if (self->v.spawnflags & SPAWNFLAG_PUSH_ONCE)
    utl_remove(self);
}

void spawn_trigger_push(edict_t *self) {
  trigger_init(self);
  self->v.touch = push_touch;
  if (!self->v.speed)
    self->v.speed = 1000;
}

static void monsterjump_touch(edict_t *self, edict_t *other) {
  if ((other->v.flags & (FL_MONSTER | FL_FLY | FL_SWIM)) != FL_MONSTER)
    return;

  // set XY even if not on ground, so the jump will clear lips
  other->v.velocity.x = self->v.avelocity.x * self->v.speed;
  other->v.velocity.y = self->v.avelocity.y * self->v.speed;

  if (!(other->v.flags & FL_ONGROUND))
    return;

  other->v.flags &= ~FL_ONGROUND;

  other->v.velocity.z = self->v.velocity.z;
}

void spawn_trigger_monsterjump(edict_t *self) {
  if (!self->v.speed)
    self->v.speed = 200;

  if (!self->v.velocity.z)
    self->v.velocity.z = TO_FIX32(200); // height

  if (!self->v.angles.x && !self->v.angles.y && !self->v.angles.z)
    self->v.angles.y = TO_DEG16(360);

  trigger_init(self);

  self->v.touch = monsterjump_touch;
}

static void changelevel_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  utl_usetargets(self, other);

  const char *nextmap = gs.worldmodel->strings + self->v.extra_trigger.string;

  if (self->v.spawnflags & SPAWNFLAG_NO_INTERMISSION) {
    G_RequestMap(nextmap);
    return;
  }

  self->v.touch = NULL;

  // TODO: intermission
  G_RequestMap(nextmap);
}

void spawn_trigger_changelevel(edict_t *self) {
  trigger_init(self);
  self->v.touch = changelevel_touch;
}
