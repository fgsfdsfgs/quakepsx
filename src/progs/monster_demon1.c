#include "prcommon.h"

enum demon_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8, STAND9,
  STAND10, STAND11, STAND12, STAND13,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7, WALK8,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6,

  LEAP1, LEAP2, LEAP3, LEAP4, LEAP5, LEAP6, LEAP7, LEAP8, LEAP9, LEAP10,
  LEAP11, LEAP12,

  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5, PAIN6,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8, DEATH9,

  ATTACKA1, ATTACKA2, ATTACKA3, ATTACKA4, ATTACKA5, ATTACKA6, ATTACKA7, ATTACKA8,
  ATTACKA9, ATTACKA10, ATTACKA11, ATTACKA12, ATTACKA13, ATTACKA14, ATTACKA15,
};

enum demon_states_e {
  MSTATE_DIE = MSTATE_EXTRA,
  MSTATE_PAIN,
};

static void demon_stand(edict_t *self);
static void demon_walk(edict_t *self);
static void demon_run(edict_t *self);
static void demon_attack(edict_t *self);
static void demon_leap(edict_t *self);
static void demon_pain(edict_t *self);
static void demon_die(edict_t *self);

static void demon_start_die(edict_t *self, edict_t *killer);
static void demon_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean demon_check_attack(edict_t *self);

static const monster_state_t monster_demon_states[MSTATE_MAX] = {
  /* STAND   */ { demon_stand,  STAND1,   STAND13   },
  /* WALK    */ { demon_walk,   WALK1,    WALK8     },
  /* RUN     */ { demon_run,    RUN1,     RUN6      },
  /* MISSILE */ { demon_leap,   LEAP1,    LEAP10    },
  /* MELEE   */ { demon_attack, ATTACKA1, ATTACKA15 },
  /* DIE     */ { demon_die,    DEATH1,   DEATH9    },
  /* PAIN    */ { demon_pain,   PAIN1,    PAIN6     },
};

static const monster_class_t monster_demon_class = {
  .state_table = monster_demon_states,
  .fn_start_pain = demon_start_pain,
  .fn_start_die = demon_start_die,
  .fn_check_attack = demon_check_attack,
  .sight_sound = SFXID_DOG_DSIGHT
};

static void demon_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  ai_stand(self);
}

static void demon_walk(edict_t *self) {
  static const s8 walktab[] = {
    8, 6, 6, 7, 4, 6, 10, 10
  };

  monster_looping_state(self, MSTATE_WALK);

  if (self->v.frame == WALK1 && (xrand32() < FTOX(0.2)))
    utl_sound(self, CHAN_VOICE, SFXID_DEMON_IDLE1, SND_MAXVOL, ATTN_IDLE);

  ai_walk(self, TO_FIX32(walktab[self->v.frame - WALK1]));
}

static void demon_run(edict_t *self) {
  static const s8 runtab[] = {
    20, 15, 36, 20, 15, 36
  };

  monster_looping_state(self, MSTATE_RUN);

  if (self->v.frame == WALK1 && (xrand32() < FTOX(0.2)))
    utl_sound(self, CHAN_VOICE, SFXID_DEMON_IDLE1, SND_MAXVOL, ATTN_IDLE);

  ai_run(self, TO_FIX32(runtab[self->v.frame - RUN1]));
}

static void demon_melee(edict_t *self, const s16 side) {
  monster_fields_t *mon = self->v.monster;

  ai_face(self);
  ai_walkmove(self, mon->ideal_yaw, TO_FIX32(12));  // allow a little closing

  x32vec3_t delta;
  XVecSub(&mon->enemy->v.origin, &self->v.origin, &delta);
  if (XVecLengthSqrIntL(&delta) > 100 * 100)
    return;

  if (!utl_can_damage(self, mon->enemy, self))
    return;

  utl_sound(self, CHAN_WEAPON, SFXID_DEMON_DHIT2, SND_MAXVOL, ATTN_NORM);

  const s16 ldmg = 10 + XMUL16(5, xrand32());
  utl_damage(mon->enemy, self, self, ldmg);

  utl_makevectors(&self->v.angles);
  const x32vec3_t org = {{
    self->v.origin.x + pr.v_forward.x * 16 + side * pr.v_right.x,
    self->v.origin.y + pr.v_forward.y * 16 + side * pr.v_right.y,
    self->v.origin.z + pr.v_forward.z * 16 + side * pr.v_right.z
  }};
  fx_spawn_blood(&org, ldmg);
}

static void demon_attack(edict_t *self) {
  static const s8 disttab[] = {
    4, 0, 0, 1, 2, 1, 6, 8,
    4, 2, -1, 5, 8, 4, 4
  };

  monster_finite_state(self, MSTATE_MELEE, MSTATE_RUN);

  switch (self->v.frame) {
  case ATTACKA5:  demon_melee(self, +200); break;
  case ATTACKA11: demon_melee(self, -200); break;
  default: break;
  }
}

static void demon_leap_touch(edict_t *self, edict_t *other) {
  if (self->v.health <= 0)
    return;

  s16 ldmg;
  if (other && (other->v.flags & FL_TAKEDAMAGE)) {
    if (XVecLengthSqrIntL(&self->v.velocity) > (400 * 400)) {
      ldmg = 40 + (rand() % 10);
      utl_damage(other, self, self, ldmg);
    }
  }

  if (!G_CheckBottom(self)) {
    if (self->v.flags & FL_ONGROUND) {
      // jump randomly to not get hung up
      self->v.touch = NULL;
      monster_set_next_state(self, MSTATE_MISSILE);
    }
    return; // not on ground yet
  }

  self->v.touch = NULL;
  monster_set_next_state(self, MSTATE_MISSILE);
  self->v.monster->next_frame = LEAP11;
}

static void demon_leap(edict_t *self) {
  monster_finite_state(self, MSTATE_MISSILE, MSTATE_RUN);

  if (self->v.frame <= LEAP4)
    ai_face(self);

  if (self->v.frame == LEAP4) {
    utl_makevectors(&self->v.angles);
    self->v.velocity.x = 600 * pr.v_forward.x;
    self->v.velocity.y = 600 * pr.v_forward.y;
    self->v.velocity.z = 600 * pr.v_forward.z + TO_FIX32(250);
    self->v.origin.z += ONE;
    self->v.touch = demon_leap_touch;
    self->v.flags &= ~FL_ONGROUND;
  } else if (self->v.frame == LEAP10) {
    // if three seconds pass without touch firing, assume demon is stuck and jump again
    self->v.monster->next_frame = LEAP1;
    self->v.nextthink = gs.time + TO_FIX32(3);
  }
}

static void demon_pain(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN, MSTATE_RUN);
}

static void demon_die(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE, -1);
  if (self->v.frame == DEATH1)
    utl_sound(self, CHAN_VOICE, SFXID_DEMON_DDEATH, SND_MAXVOL, ATTN_NORM);
  else if (self->v.frame == DEATH6)
    self->v.solid = SOLID_NOT;
}

static void demon_start_die(edict_t *self, edict_t *killer) {
  if (self->v.health < -80) {
    ai_gib(self);
    return;
  }
  monster_exec_state(self, MSTATE_DIE);
}

static void demon_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  if (self->v.touch == demon_leap_touch)
    return;

  if (self->v.monster->pain_finished > gs.time)
    return;

  self->v.monster->pain_finished = gs.time + ONE;
  utl_sound(self, CHAN_VOICE, SFXID_DEMON_DPAIN1, SND_MAXVOL, ATTN_NORM);

  if (XMUL16(xrand32(), 200) > damage)
    return; // didn't flinch

  monster_exec_state(self, MSTATE_PAIN);
}

static inline qboolean demon_check_melee(edict_t *self) {
  return (pr.enemy_range == RANGE_MELEE);
}

static qboolean demon_check_jump(edict_t *self) {
  edict_t *enemy = self->v.monster->enemy;

  if ((self->v.origin.z + self->v.mins.z) > (enemy->v.origin.z + enemy->v.mins.z + 3 * (enemy->v.size.z >> 2)))
    return false;

  if ((self->v.origin.z + self->v.maxs.z) < (enemy->v.origin.z + enemy->v.mins.z + 1 * (enemy->v.size.z >> 2)))
    return false;

  x32vec3_t delta;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  delta.z = 0;

  const s32 d = XVecLengthSqrIntL(&delta);

  if (d < 100 * 100)
    return false;

  if (d > 200 * 200) {
    if (xrand32() < FTOX(0.9))
      return false;
  }

  return true;
}

static qboolean demon_check_attack(edict_t *self) {
  // if close enough for slashing, go for it
  if (demon_check_melee(self)) {
    self->v.monster->attack_state = AS_MELEE;
    return true;
  }

  if (demon_check_jump(self)) {
    self->v.monster->attack_state = AS_MISSILE;
    utl_sound(self, CHAN_VOICE, SFXID_DEMON_DJUMP, SND_MAXVOL, ATTN_NORM);
    return true;
  }

  return false;
}

void spawn_monster_demon1(edict_t *self) {
  G_SetModel(self, MDLID_DEMON);
  self->v.mins = gs.worldmodel->hulls[2].mins;
  self->v.maxs = gs.worldmodel->hulls[2].maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.health = self->v.max_health = 300;
  walkmonster_start(self, &monster_demon_class);
}
