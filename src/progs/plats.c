#include "prcommon.h"

#define SF_PLAT_LOW_TRIGGER 1

static void plat_hit_bottom(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_PLATS_MEDPLAT2, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_BOTTOM;
}

static void plat_go_down(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_PLATS_MEDPLAT1, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_DOWN;
  utl_calc_move(self, &self->v.door->pos2, self->v.speed, plat_hit_bottom);
}

static void plat_hit_top(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_PLATS_MEDPLAT2, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_TOP;
  self->v.think = plat_go_down;
  self->v.nextthink = self->v.ltime + TO_FIX32(2);
}

static void plat_go_up(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_PLATS_MEDPLAT1, SND_MAXVOL, ATTN_NORM);
  self->v.door->state = STATE_UP;
  utl_calc_move(self, &self->v.door->pos1, self->v.speed, plat_hit_top);
}

static void plat_crush(edict_t *self, edict_t *other) {
  utl_damage(other, self, self, 1);

  if (self->v.door->state == STATE_UP)
    plat_go_down(self);
  else if (self->v.door->state == STATE_DOWN)
    plat_go_up(self);
}

static void plat_use(edict_t *self, edict_t *activator) {
  self->v.use = NULL;
  if (self->v.door->state == STATE_UP)
    plat_go_down(self);
}

static void plat_trigger_use(edict_t *self, edict_t *activator) {
  if (self->v.think)
    return; // already activated
  plat_go_down(self);
}

static void plat_trigger_touch(edict_t *self, edict_t *other) {
  if (other->v.classname != ENT_PLAYER)
    return;
  if (other->v.health <= 0)
    return;
  self = self->v.owner;
  if (self->v.door->state == STATE_BOTTOM)
    plat_go_up(self);
  else if (self->v.door->state == STATE_TOP)
    self->v.nextthink = self->v.ltime + ONE; // delay going down
}

static void spawn_plat_trigger(edict_t *self) {
  edict_t *t = ED_Alloc();
  t->v.classname = ENT_TEMP_ENTITY;
  t->v.touch = plat_trigger_touch;
  t->v.solid = SOLID_TRIGGER;
  t->v.owner = self;

  t->v.maxs.x = self->v.maxs.x - TO_FIX32(25);
  t->v.maxs.y = self->v.maxs.y - TO_FIX32(25);
  t->v.maxs.z = self->v.maxs.z + TO_FIX32(8);
  t->v.mins.x = self->v.mins.x + TO_FIX32(25);
  t->v.mins.y = self->v.mins.y + TO_FIX32(25);
  t->v.mins.z = t->v.maxs.z - (self->v.door->pos1.z - self->v.door->pos2.z + TO_FIX32(8));

  if (self->v.spawnflags & SF_PLAT_LOW_TRIGGER)
    t->v.maxs.z = t->v.mins.z + TO_FIX32(8);

  if (self->v.size.x <= TO_FIX32(50)) {
    t->v.mins.x = (self->v.mins.x + self->v.maxs.x) >> 1;
    t->v.maxs.x = t->v.mins.x + ONE;
  }
  if (self->v.size.y <= TO_FIX32(50)) {
    t->v.mins.y = (self->v.mins.y + self->v.maxs.y) >> 1;
    t->v.maxs.y = t->v.mins.y + ONE;
  }

  G_SetSize(t, &t->v.mins, &t->v.maxs);
  G_LinkEdict(t, false);
}

void spawn_func_plat(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.blocked = plat_crush;
  self->v.use = plat_trigger_use;

  if (!self->v.speed)
    self->v.speed = 150;

  x32 height = 0;
  if (self->v.velocity.z) {
    height = self->v.velocity.z;
    self->v.velocity.z = 0;
  }

  door_init(self);

  // pos1 is the top position, pos2 is the bottom
  self->v.door->pos1 = self->v.origin;
  self->v.door->pos2 = self->v.origin;
  if (height)
    self->v.door->pos2.z = self->v.origin.z - height;
  else
    self->v.door->pos2.z = self->v.origin.z - self->v.size.z - TO_FIX32(8);

  spawn_plat_trigger(self);

  if (self->v.targetname) {
    self->v.door->state = STATE_UP;
    self->v.use = plat_use;
  } else {
    self->v.origin = self->v.door->pos2;
    self->v.door->state = STATE_BOTTOM;
  }
}

void spawn_func_train(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
}
