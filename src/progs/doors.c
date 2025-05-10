#include "prcommon.h"

#define SF_START_OPEN 1
#define SF_DONT_LINK 4
#define SF_GOLD_KEY 8
#define SF_SILVER_KEY 16
#define SF_TOGGLE 32

#define SF_SECRET_OPEN_ONCE 1 // stays open
#define SF_SECRET_1ST_LEFT 2  // 1st move is left of arrow
#define SF_SECRET_1ST_DOWN 4  // 1st move is down from arrow
#define SF_SECRET_NO_SHOOT 8  // only opened by trigger
#define SF_SECRET_YES_SHOOT 16 // shootable even if targeted

enum door_state_e {
  STATE_TOP,
  STATE_BOTTOM,
  STATE_UP,
  STATE_DOWN
};

static void door_touch(edict_t *self, edict_t *other);
static void door_blocked(edict_t *self, edict_t *other);
static void door_use(edict_t *self, edict_t *other);
static void door_killed(edict_t *self, edict_t *killer);
static void door_fire(edict_t *self, edict_t *activator);
static void door_go_down(edict_t *self);
static void door_go_up(edict_t *self, edict_t *activator);

static void door_trigger_touch(edict_t *self, edict_t *other) {
  if (other->v.health <= 0)
    return;

  self = self->v.owner;

  if (gs.time < self->v.door->touch_finished)
    return;

  self->v.door->touch_finished = gs.time + ONE;

  door_use(self, other);
}

static edict_t *spawn_door_trigger(edict_t *self, const x32vec3_t *cmins, const x32vec3_t *cmaxs) {
  edict_t *t = ED_Alloc();
  t->v.classname = ENT_TEMP_ENTITY;
  t->v.movetype = MOVETYPE_NONE;
  t->v.solid = SOLID_TRIGGER;
  t->v.owner = self;
  t->v.touch = door_trigger_touch;
  t->v.mins.x = cmins->x - TO_FIX32(60);
  t->v.mins.y = cmins->y - TO_FIX32(60);
  t->v.mins.z = cmins->z - TO_FIX32(8);
  t->v.maxs.x = cmaxs->x + TO_FIX32(60);
  t->v.maxs.y = cmaxs->y + TO_FIX32(60);
  t->v.maxs.z = cmaxs->z + TO_FIX32(8);
  G_SetSize(t, &t->v.mins, &t->v.maxs);
  G_LinkEdict(t, false);
}

static void door_init(edict_t *self) {
  const x32 wait = self->v.extra_trigger.wait; // save this because it's the same field as `door`
  self->v.door = Mem_ZeroAlloc(sizeof(*self->v.door));
  self->v.door->linked = gs.edicts;
  self->v.door->field = gs.edicts;
  self->v.door->wait = wait;
  self->v.owner = gs.edicts;
}

static void door_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  edict_t *owner = self->v.owner;

  if (owner->v.door->touch_finished > gs.time)
    return;

  owner->v.door->touch_finished = gs.time + ONE;

  if (owner->v.extra_trigger.string) {
    utl_sound(other, CHAN_VOICE, SFXID_MISC_TALK, SND_MAXVOL, ATTN_NORM);
    Scr_SetCenterMsg(gs.worldmodel->strings + owner->v.extra_trigger.string);
  }

  // key door stuff

  if ((owner->v.spawnflags & (SF_GOLD_KEY | SF_SILVER_KEY)) == 0)
    return;

  if (owner->v.spawnflags & SF_SILVER_KEY) {
    if ((other->v.player->stats.items & IT_KEY1) == 0) {
      utl_sound(other, CHAN_VOICE, SFXID_DOORS_MEDTRY, SND_MAXVOL, ATTN_NORM);
      Scr_SetCenterMsg("You need the silver key");
      return;
    } else {
      other->v.player->stats.items &= ~IT_KEY1;
    }
  } else if (owner->v.spawnflags & SF_GOLD_KEY) {
    if ((other->v.player->stats.items & IT_KEY2) == 0) {
      utl_sound(other, CHAN_VOICE, SFXID_DOORS_MEDTRY, SND_MAXVOL, ATTN_NORM);
      Scr_SetCenterMsg("You need the gold key");
      return;
    } else {
      other->v.player->stats.items &= ~IT_KEY2;
    }
  }

  self->v.touch = NULL;
  if (self->v.door->linked != gs.edicts)
    self->v.door->linked->v.touch = NULL; // get paired door

  door_use(self, other);
}

static void door_blocked(edict_t *self, edict_t *other) {
  utl_damage(other, self, self, self->v.dmg);

  // if a door has a negative wait, it would never come back if blocked,
  // so let it just squash the object to death real fast
  if (self->v.door->wait >= 0) {
    if (self->v.door->state == STATE_DOWN)
      door_go_up(self, other);
    else
      door_go_down(self);
  }
}

static void door_use(edict_t *self, edict_t *other) {
  self->v.extra_trigger.string = 0; // door message are for touch only
  self->v.owner->v.extra_trigger.string = 0;
  self->v.door->linked->v.extra_trigger.string = 0;
  door_fire(self->v.owner, other);
}

static void door_killed(edict_t *self, edict_t *killer) {
  self = self->v.owner;
  self->v.health = self->v.max_health;
  self->v.flags &= ~FL_TAKEDAMAGE; // will be reset upon return
  door_use(self, killer);
}

static void door_fire(edict_t *self, edict_t *activator) {
  // play use key sound
  if (self->v.spawnflags & (SF_GOLD_KEY | SF_SILVER_KEY))
    utl_sound(self, CHAN_VOICE, SFXID_DOORS_MEDUSE, SND_MAXVOL, ATTN_NORM);

  self->v.extra_trigger.string = 0; // no more message

  edict_t *starte;

  if (self->v.spawnflags & SF_TOGGLE) {
    if (self->v.door->state == STATE_UP || self->v.door->state == STATE_TOP) {
      starte = self;
      do {
        door_go_down(self);
        self = self->v.door->linked;
      } while (self != starte && self != gs.edicts);
      return;
    }
  }

  // trigger all paired doors
  starte = self;
  do {
    door_go_up(self, activator);
    self = self->v.door->linked;
  } while (self != starte && self != gs.edicts);
}

static void door_hit_top(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_TOP;
  if (self->v.spawnflags & SF_TOGGLE)
    return; // don't come down automatically
  self->v.think = door_go_down;
  self->v.nextthink = self->v.ltime + self->v.door->wait;
}

static void door_hit_bottom(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_BOTTOM;
}

static void door_go_down(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);

  if (self->v.max_health) {
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.health = self->v.max_health;
  }

  self->v.door->state = STATE_DOWN;
  utl_calc_move(self, &self->v.door->pos1, self->v.speed, door_hit_bottom);
}

static void door_go_up(edict_t *self, edict_t *activator) {
  if (self->v.door->state == STATE_UP)
    return; // already going up

  if (self->v.door->state == STATE_TOP) {
     // reset top wait time
     self->v.nextthink = self->v.ltime + self->v.door->wait;
     return;
  }

  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_UP;
  utl_calc_move(self, &self->v.door->pos2, self->v.speed, door_hit_top);
  utl_usetargets(self, activator);
}

static qboolean doors_touching(edict_t *e1, edict_t *e2) {
  if (e1->v.mins.x > e2->v.maxs.x)
    return false;
  if (e1->v.mins.y > e2->v.maxs.y)
    return false;
  if (e1->v.mins.z > e2->v.maxs.z)
    return false;
  if (e1->v.maxs.x < e2->v.mins.x)
    return false;
  if (e1->v.maxs.y < e2->v.mins.y)
    return false;
  if (e1->v.maxs.z < e2->v.mins.z)
    return false;
  return true;
}

static void door_link(edict_t *self) {
  if (self->v.door->linked != gs.edicts)
    return; // already linked by another door

  if (self->v.spawnflags & SF_DONT_LINK) {
    self->v.owner = self->v.door->linked = self;
    return; // don't want to link this door
  }

  x32vec3_t cmins = self->v.mins;
  x32vec3_t cmaxs = self->v.maxs;

  edict_t *starte = self;
  edict_t *t = self;

  while (true) {
    self->v.owner = starte; // master door

    if (self->v.health)
      starte->v.health = self->v.health;

    if (self->v.targetname)
      starte->v.targetname = self->v.targetname;

    if (self->v.extra_trigger.string)
      starte->v.extra_trigger.string = self->v.extra_trigger.string;

    t = G_FindByClassname(t, self->v.classname);
    if (t == gs.edicts) {
      self->v.door->linked = starte; // make the chain a loop
      // shootable, fired, or key doors just needed the owner/enemy links,
      // they don't spawn a field
      self = self->v.owner;
      if (self->v.health)
        return;
      if (self->v.targetname)
        return;
      if (self->v.spawnflags & (SF_GOLD_KEY | SF_SILVER_KEY))
        return;
      self->v.owner->v.door->field = spawn_door_trigger(self, &cmins, &cmaxs);
      return;
    }

    if (doors_touching(self, t)) {
      if (t->v.door->linked != gs.edicts)
        return; // cross connected doors

      self->v.door->linked = t;
      self = t;

      if (t->v.mins.x < cmins.x)
        cmins.x = t->v.mins.x;
      if (t->v.mins.y < cmins.y)
        cmins.y = t->v.mins.y;
      if (t->v.mins.z < cmins.z)
        cmins.z = t->v.mins.z;

      if (t->v.maxs.x > cmaxs.x)
        cmaxs.x = t->v.maxs.x;
      if (t->v.maxs.y > cmaxs.y)
        cmaxs.y = t->v.maxs.y;
      if (t->v.maxs.z > cmaxs.z)
        cmaxs.z = t->v.maxs.z;
    }
  }
}

void spawn_func_door(edict_t *self) {
  // TODO: noise
  utl_set_movedir(self, &self->v.avelocity);

  self->v.max_health = self->v.health;
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.blocked = door_blocked;
  self->v.use = door_use;
  self->v.touch = door_touch;

  if (!self->v.speed)
    self->v.speed = 100;

  if (!self->v.extra_trigger.wait)
    self->v.extra_trigger.wait = TO_FIX32(3); // this will get moved to v.door->wait

  if (!self->v.count)
    self->v.count = 8; // lip

  if (!self->v.dmg)
    self->v.dmg = 2;

  door_init(self);

  const x32 mult = abs(XVecDotSL(&self->v.avelocity, &self->v.size) - TO_FIX32(self->v.count));
  self->v.door->pos1 = self->v.origin;
  self->v.door->pos2.x = self->v.door->pos1.x + xmul32(self->v.avelocity.x, mult);
  self->v.door->pos2.y = self->v.door->pos1.y + xmul32(self->v.avelocity.y, mult);
  self->v.door->pos2.z = self->v.door->pos1.z + xmul32(self->v.avelocity.z, mult);

  // SF_START_OPEN is to allow an entity to be lighted in the closed position
  // but spawn in the open position
  if (self->v.spawnflags & SF_START_OPEN) {
    self->v.origin = self->v.door->pos2;
    self->v.door->pos2 = self->v.door->pos1;
    self->v.door->pos1 = self->v.origin;
  }

  self->v.door->state = STATE_BOTTOM;

  if (self->v.health) {
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.th_die = door_killed;
  }

  if (self->v.spawnflags & (SF_GOLD_KEY | SF_SILVER_KEY))
    self->v.door->wait = -1;

  // LinkDoors can't be done until all of the doors have been spawned, so
  // the sizes can be detected properly.
  self->v.think = door_link;
  self->v.nextthink = self->v.ltime + PR_FRAMETIME;
}

static void secret_done(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
  if (!self->v.targetname || (self->v.spawnflags & SF_SECRET_YES_SHOOT)) {
    self->v.health = 1;
    self->v.flags |= FL_TAKEDAMAGE;
  }
}

static void secret_move6(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);
  utl_calc_move(self, &self->v.door->start, self->v.speed, secret_done);
}

static void secret_move5(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
  self->v.nextthink = self->v.ltime + ONE;
  self->v.think = secret_move6;
}

static void secret_move4(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);
  utl_calc_move(self, &self->v.door->pos1, self->v.speed, secret_move5);
}

static void secret_move3(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
  if ((self->v.spawnflags & SF_SECRET_OPEN_ONCE) == 0) {
    self->v.nextthink = self->v.ltime + self->v.door->wait;
    self->v.think = secret_move4;
  }
}

static void secret_move2(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);
  utl_calc_move(self, &self->v.door->pos2, self->v.speed, secret_move3);
}

static void secret_move1(edict_t *self) {
  self->v.nextthink = self->v.ltime + ONE;
  self->v.think = secret_move2;
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DRCLOS4, SND_MAXVOL, ATTN_NORM);
}

static void secret_use(edict_t *self, edict_t *activator) {
  self->v.health = 1;

  // exit if still moving around...
  if (self->v.origin.x != self->v.oldorigin.x || self->v.origin.y != self->v.oldorigin.y || self->v.origin.z != self->v.oldorigin.z)
    return;

  self->v.extra_trigger.string = 0; // no more message

  utl_usetargets(self, activator);

  if ((self->v.spawnflags & SF_SECRET_NO_SHOOT) == 0)
    self->v.flags &= ~FL_TAKEDAMAGE; // will be reset upon return

  XVecZero(&self->v.velocity);

  // Make a sound, wait a little...
  utl_sound(self, CHAN_VOICE, SFXID_BUTTONS_SWITCH04, SND_MAXVOL, ATTN_NORM);
  self->v.nextthink = self->v.ltime + PR_FRAMETIME;

  const s32 temp = 1 - (self->v.spawnflags & SF_SECRET_1ST_LEFT); // 1 or -1
  utl_makevectors(&self->v.angles);

  // NOTE: these are overridable fields in the original quake, but no official maps seem to actually do it?
  x32 t_width;
  if (self->v.spawnflags & SF_SECRET_1ST_DOWN)
    t_width = abs(XVecDotSL(&pr.v_up, &self->v.size));
  else
    t_width = abs(XVecDotSL(&pr.v_right, &self->v.size));
  const x32 t_length = abs(XVecDotSL(&pr.v_forward, &self->v.size));

  if (self->v.flags & SF_SECRET_1ST_DOWN) {
    self->v.door->pos1.x = self->v.origin.x - xmul32(pr.v_up.x, t_width);
    self->v.door->pos1.y = self->v.origin.y - xmul32(pr.v_up.y, t_width);
    self->v.door->pos1.z = self->v.origin.z - xmul32(pr.v_up.z, t_width);
  } else {
    self->v.door->pos1.x = self->v.origin.x + xmul32(pr.v_right.x, t_width * temp);
    self->v.door->pos1.y = self->v.origin.y + xmul32(pr.v_right.y, t_width * temp);
    self->v.door->pos1.z = self->v.origin.z + xmul32(pr.v_right.z, t_width * temp);
  }

  self->v.door->pos2.x = self->v.door->pos1.x + xmul32(pr.v_forward.x, t_length);
  self->v.door->pos2.y = self->v.door->pos1.y + xmul32(pr.v_forward.y, t_length);
  self->v.door->pos2.z = self->v.door->pos1.z + xmul32(pr.v_forward.z, t_length);

  utl_calc_move(self, &self->v.door->pos1, self->v.speed, secret_move1);
  utl_sound(self, CHAN_VOICE, SFXID_DOORS_DOORMV1, SND_MAXVOL, ATTN_NORM);
}

static void secret_blocked(edict_t *self, edict_t *other) {
  if (self->v.door->touch_finished > gs.time)
    return;

  self->v.door->touch_finished = gs.time + FTOX(0.5);

  utl_damage(other, self, self, self->v.dmg);
}

static void secret_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;

  if (self->v.door->touch_finished > gs.time)
    return;

  self->v.door->touch_finished = gs.time + TO_FIX32(2);

  if (self->v.extra_trigger.string) {
    utl_sound(other, CHAN_VOICE, SFXID_MISC_TALK, SND_MAXVOL, ATTN_NORM);
    Scr_SetCenterMsg(gs.worldmodel->strings + self->v.extra_trigger.string);
  }
}

void spawn_func_door_secret(edict_t *self) {
  // TODO: noise
  self->v.max_health = self->v.health;
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.speed = 50;
  self->v.touch = secret_touch;
  self->v.blocked = secret_blocked;
  self->v.use = secret_use;

  if (!self->v.extra_trigger.wait)
    self->v.extra_trigger.wait = TO_FIX32(5); // this will get moved to v.door->wait

  if (!self->v.dmg)
    self->v.dmg = 2;

  door_init(self);

  if (!self->v.targetname || (self->v.spawnflags & SF_SECRET_YES_SHOOT)) {
    self->v.health = 1;
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.th_die = secret_use;
  }

  self->v.door->start = self->v.origin;
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
