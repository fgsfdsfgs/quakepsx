#include "prcommon.h"

enum dog_frames_e {
  ATTACK1, ATTACK2, ATTACK3, ATTACK4, ATTACK5, ATTACK6, ATTACK7, ATTACK8,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8, DEATH9,

  DEATHB1, DEATHB2, DEATHB3, DEATHB4, DEATHB5, DEATHB6, DEATHB7, DEATHB8,
  DEATHB9,

  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5, PAIN6,

  PAINB1, PAINB2, PAINB3, PAINB4, PAINB5, PAINB6, PAINB7, PAINB8, PAINB9, PAINB10,
  PAINB11, PAINB12, PAINB13, PAINB14, PAINB15, PAINB16,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6, RUN7, RUN8, RUN9, RUN10, RUN11, RUN12,

  LEAP1, LEAP2, LEAP3, LEAP4, LEAP5, LEAP6, LEAP7, LEAP8, LEAP9,

  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8, STAND9,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7, WALK8
};

static void dog_stand(edict_t *self);
static void dog_walk(edict_t *self);
static void dog_run(edict_t *self);
static void dog_melee(edict_t *self);
static void dog_leap(edict_t *self);
static void dog_pain_a(edict_t *self);
static void dog_pain_b(edict_t *self);
static void dog_die_a(edict_t *self);
static void dog_die_b(edict_t *self);

static void dog_start_die(edict_t *self, edict_t *killer);
static void dog_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean dog_check_attack(edict_t *self);

static const monster_state_t monster_dog_states[MSTATE_COUNT] = {
  /* STAND   */ { dog_stand,  STAND1,  STAND9   },
  /* WALK    */ { dog_walk,   WALK1,   WALK8    },
  /* RUN     */ { dog_run,    RUN1,    RUN12    },
  /* MISSILE */ { dog_leap,   LEAP1,   LEAP9    },
  /* MELEE   */ { dog_melee,  ATTACK1, ATTACK8  },
  /* DIE_A   */ { dog_die_a,  DEATH1,  DEATH9   },
  /* DIE_B   */ { dog_die_b,  DEATHB1, DEATHB9  },
  /* PAIN_A  */ { dog_pain_a, PAIN1,   PAIN6    },
  /* PAIN_B  */ { dog_pain_b, PAINB1,  PAINB16  },
  /* PAIN_C  */ { NULL,       -1,      -1       },
  /* EXTRA   */ { NULL,       -1,      -1       },
};

static const monster_class_t monster_dog_class = {
  .state_table = monster_dog_states,
  .fn_start_pain = dog_start_pain,
  .fn_start_die = dog_start_die,
  .fn_check_attack = dog_check_attack,
  .sight_sound = SFXID_DOG_DSIGHT
};

static void dog_stand(edict_t *self) {
  ai_stand(self);
  monster_loop_state(self, MSTATE_STAND);
}

static void dog_walk(edict_t *self) {
  if (self->v.frame == WALK1) {
    if (xrand32() < FTOX(0.2))
      Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_DOG_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
  }
  ai_walk(self, TO_FIX32(8));
  monster_loop_state(self, MSTATE_WALK);
}

static void dog_run(edict_t *self) {
  // distance to move each frame
  static const s8 runtable[] = {
    16, 32, 32, 20, 64, 32, 16, 32, 32, 20, 64, 32
  };

  if (self->v.frame == RUN1) {
    if (xrand32() < FTOX(0.2))
      Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_DOG_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
  }

  ai_run(self, TO_FIX32(runtable[self->v.frame - RUN1]));

  monster_loop_state(self, MSTATE_RUN);
}

static void dog_bite(edict_t *self) {
  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_DOG_DATTACK1, &self->v.origin, SND_MAXVOL, ATTN_NORM);

  edict_t *enemy = self->v.monster->enemy;
  if (!enemy)
    return;

  ai_charge(self, TO_FIX32(10));

  x32vec3_t delta;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  const s32 d = XVecLengthSqrIntL(&delta);
  if (d > 100 * 100)
    return;

  const s16 ldmg = (xrand32() + xrand32() + xrand32()) / 512;
  utl_damage(enemy, self, self, ldmg);
}

static void dog_melee(edict_t *self) {
  if (self->v.frame == ATTACK4)
    dog_bite(self);
  else
    ai_charge(self, TO_FIX32(10));
  monster_end_state(self, MSTATE_MELEE, MSTATE_RUN);
}

static void dog_leap_touch(edict_t *self, edict_t *other) {
  if (self->v.health <= 0)
    return;

  s16 ldmg;
  if (other && other->v.flags & FL_TAKEDAMAGE) {
    if (XVecLengthSqrIntL(&self->v.velocity) > (300 * 300)) {
      ldmg = 10 + (rand() % 10);
      utl_damage(other, self, self, ldmg);
    }
  }

  if (!G_CheckBottom(self)) {
    if (self->v.flags & FL_ONGROUND) {
      // jump randomly to not get hung up
      self->v.touch = null_touch;
      monster_set_state(self, MSTATE_MISSILE);
    }
    return; // not on ground yet
  }

  self->v.touch = null_touch;
  monster_set_state(self, MSTATE_RUN);
}

static void dog_leap(edict_t *self) {
  if (self->v.frame == LEAP1) {
    ai_face(self);
  } else if (self->v.frame == LEAP2) {
    ai_face(self);
    self->v.touch = dog_leap_touch;
    // jump
    utl_makevectors(&self->v.angles);
    XVecScaleInt(&pr.v_forward, 300, &self->v.velocity);
    self->v.velocity.z += TO_FIX32(200);
    self->v.flags &= ~FL_ONGROUND;
  }
  // leap until we actually hit something
  monster_end_state(self, MSTATE_MISSILE, -1);
}

static void dog_pain_a(edict_t *self) {
  monster_end_state(self, MSTATE_PAIN_A, MSTATE_RUN);
}

static void dog_pain_b(edict_t *self) {
  switch (self->v.frame) {
  case PAINB3:  ai_pain(self, TO_FIX32(4)); break;
  case PAINB4:  ai_pain(self, TO_FIX32(12)); break;
  case PAINB5:  ai_pain(self, TO_FIX32(12)); break;
  case PAINB6:  ai_pain(self, TO_FIX32(2)); break;
  case PAINB8:  ai_pain(self, TO_FIX32(4)); break;
  case PAINB10: ai_pain(self, TO_FIX32(10)); break;
  default: break;
  }

  monster_end_state(self, MSTATE_PAIN_B, MSTATE_RUN);
}

static void dog_die_a(edict_t *self) {
  monster_end_state(self, MSTATE_DIE_A, -1);
}

static void dog_die_b(edict_t *self) {
  monster_end_state(self, MSTATE_DIE_B, -1);
}

static void dog_start_die(edict_t *self, edict_t *killer) {
  // check for gib
  if (self->v.health < -35) {
    ai_gib(self);
    return;
  }

  // regular death
  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_DOG_DDEATH, &self->v.origin, SND_MAXVOL, ATTN_NORM);
  self->v.solid = SOLID_NOT;
  monster_set_state(self, (xrand32() > HALF) ? MSTATE_DIE_A : MSTATE_DIE_B);
}

static void dog_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_DOG_DPAIN1, &self->v.origin, SND_MAXVOL, ATTN_NORM);
  monster_set_state(self, (xrand32() > HALF) ? MSTATE_PAIN_A : MSTATE_PAIN_B);
}

static inline qboolean dog_check_melee(edict_t *self) {
  return (pr.enemy_range == RANGE_MELEE);
}

static qboolean dog_check_jump(edict_t *self) {
  edict_t *enemy = self->v.monster->enemy;

  if ((self->v.origin.z + self->v.mins.z) > (enemy->v.origin.z + enemy->v.mins.z + 3 * (enemy->v.size.z >> 2)))
    return false;

  if ((self->v.origin.z + self->v.maxs.z) < (enemy->v.origin.z + enemy->v.mins.z + 1 * (enemy->v.size.z >> 2)))
    return false;

  x32vec3_t delta;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  delta.z = 0;

  const s32 d = XVecLengthSqrIntL(&delta);

  if (d < 80 * 80)
    return false;
  if (d > 150 * 150)
    return false;

  return true;
}

static qboolean dog_check_attack(edict_t *self) {
  // if close enough for slashing, go for it
  if (dog_check_melee(self)) {
    self->v.monster->attack_state = AS_MELEE;
    return true;
  }

  if (dog_check_jump(self)) {
    self->v.monster->attack_state = AS_MISSILE;
    return true;
  }

  return false;
}

void spawn_monster_dog(edict_t *self) {
  G_SetModel(self, MDLID_DOG);
  XVecSetInt(&self->v.mins, -32, -32, -24);
  XVecSetInt(&self->v.maxs, +32, +32, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.health = self->v.max_health = 25;
  walkmonster_start(self, &monster_dog_class);
}
