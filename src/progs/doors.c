#include "prcommon.h"

#define SPAWNFLAG_START_OPEN 1
#define SPAWNFLAG_DONT_LINK 4
#define SPAWNFLAG_GOLD_KEY 8
#define SPAWNFLAG_SILVER_KEY 16
#define SPAWNFLAG_TOGGLE 32

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

  if ((owner->v.spawnflags & (SPAWNFLAG_GOLD_KEY | SPAWNFLAG_SILVER_KEY)) == 0)
    return;

  if (owner->v.spawnflags & SPAWNFLAG_SILVER_KEY) {
    if ((other->v.player->stats.items & IT_KEY1) == 0) {
      utl_sound(other, CHAN_VOICE, SFXID_DOORS_MEDTRY, SND_MAXVOL, ATTN_NORM);
      Scr_SetCenterMsg("You need the silver key");
      return;
    } else {
      other->v.player->stats.items &= ~IT_KEY1;
    }
  } else if (owner->v.spawnflags & SPAWNFLAG_GOLD_KEY) {
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
  if (self->v.spawnflags & (SPAWNFLAG_GOLD_KEY | SPAWNFLAG_SILVER_KEY))
    utl_sound(self, CHAN_VOICE, SFXID_DOORS_MEDUSE, SND_MAXVOL, ATTN_NORM);

  self->v.extra_trigger.string = 0; // no more message

  edict_t *starte;

  if (self->v.spawnflags & SPAWNFLAG_TOGGLE) {
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
  if (self->v.spawnflags & SPAWNFLAG_TOGGLE)
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

  if (self->v.spawnflags & SPAWNFLAG_DONT_LINK) {
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
      if (self->v.spawnflags & (SPAWNFLAG_GOLD_KEY | SPAWNFLAG_SILVER_KEY))
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

  // SPAWNFLAG_START_OPEN is to allow an entity to be lighted in the closed position
  // but spawn in the open position
  if (self->v.spawnflags & SPAWNFLAG_START_OPEN) {
    self->v.origin = self->v.door->pos2;
    self->v.door->pos2 = self->v.door->pos1;
    self->v.door->pos1 = self->v.origin;
  }

  self->v.door->state = STATE_BOTTOM;

  if (self->v.health) {
    self->v.flags |= FL_TAKEDAMAGE;
    self->v.th_die = door_killed;
  }

  if (self->v.spawnflags & (SPAWNFLAG_GOLD_KEY | SPAWNFLAG_SILVER_KEY))
    self->v.door->wait = -1;

  // LinkDoors can't be done until all of the doors have been spawned, so
  // the sizes can be detected properly.
  self->v.think = door_link;
  self->v.nextthink = self->v.ltime + PR_FRAMETIME;
}
