#include "prcommon.h"

static void button_done(edict_t *self) {
  self->v.door->state = STATE_BOTTOM;
}

static void button_return(edict_t *self) {
  self->v.door->state = STATE_DOWN;
  self->v.frame = 0; // use normal textures
  utl_calc_move(self, &self->v.door->pos1, self->v.speed, button_done);
  if (self->v.health)
    self->v.flags |= FL_TAKEDAMAGE; // can be shot again
}

static void button_wait(edict_t *self) {
  self->v.door->state = STATE_TOP;
  self->v.nextthink = self->v.ltime + self->v.door->wait;
  self->v.think = button_return;
  utl_usetargets(self, self->v.door->linked);
  self->v.frame = 1; // use alternate textures
}

static void button_fire(edict_t *self, edict_t *activator) {
  if (self->v.door->state == STATE_UP || self->v.door->state == STATE_TOP)
    return;

  utl_sound(self, CHAN_VOICE, self->v.noise, SND_MAXVOL, ATTN_NORM);

  self->v.door->linked = activator;
  self->v.door->state = STATE_UP;
  utl_calc_move(self, &self->v.door->pos2, self->v.speed, button_wait);
}

static void button_touch(edict_t *self, edict_t *other) {
  if (other->v.classname == ENT_PLAYER)
    button_fire(self, other);
}

static void button_killed(edict_t *self, edict_t *killer) {
  self->v.health = self->v.max_health;
  self->v.flags &= ~FL_TAKEDAMAGE; // will be reset upon return
  button_fire(self, killer);
}

void spawn_func_button(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.use = button_fire;
  self->v.noise = SFXID_BUTTONS_AIRBUT1;

  utl_set_movedir(self, &self->v.avelocity);

  if (self->v.health) {
    self->v.max_health = self->v.health;
    self->v.th_die = button_killed;
    self->v.flags |= FL_TAKEDAMAGE;
  } else {
    self->v.touch = button_touch;
  }

  if (!self->v.speed)
    self->v.speed = 40;
  if (!self->v.extra_trigger.wait)
    self->v.extra_trigger.wait = ONE;
  if (!self->v.count)
    self->v.count = 4; // lip

  door_init(self);

  self->v.door->state = STATE_BOTTOM;

  const x32 length = abs(XVecDotSL(&self->v.avelocity, &self->v.size)) - TO_FIX32(self->v.count);

  self->v.door->pos1 = self->v.origin;
  self->v.door->pos2.x = self->v.door->pos1.x + xmul32(self->v.avelocity.x, length);
  self->v.door->pos2.y = self->v.door->pos1.y + xmul32(self->v.avelocity.y, length);
  self->v.door->pos2.z = self->v.door->pos1.z + xmul32(self->v.avelocity.z, length);
}

void spawn_func_illusionary(edict_t *self) {
  self->v.solid = SOLID_NOT;
}

static void func_wall_use(edict_t *self, edict_t *activator) {
  // change to alternate textures
  self->v.frame = 1 - self->v.frame;
}

void spawn_func_wall(edict_t *self) {
  self->v.solid = SOLID_BSP;
  self->v.movetype = MOVETYPE_PUSH;
  self->v.use = func_wall_use;
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
