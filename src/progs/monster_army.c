#include "prcommon.h"

enum army_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8,
  DEATH9, DEATH10, 

  DEATHC1, DEATHC2, DEATHC3, DEATHC4, DEATHC5, DEATHC6, DEATHC7, DEATHC8,
  DEATHC9, DEATHC10, DEATHC11,

  LOAD1, LOAD2, LOAD3, LOAD4, LOAD5, LOAD6, LOAD7, LOAD8, LOAD9, LOAD10, LOAD11,

  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5, PAIN6,

  PAINB1, PAINB2, PAINB3, PAINB4, PAINB5, PAINB6, PAINB7, PAINB8, PAINB9, PAINB10,
  PAINB11, PAINB12, PAINB13, PAINB14,

  PAINC1, PAINC2, PAINC3, PAINC4, PAINC5, PAINC6, PAINC7, PAINC8, PAINC9, PAINC10,
  PAINC11, PAINC12, PAINC13,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6, RUN7, RUN8,

  SHOOT1, SHOOT2, SHOOT3, SHOOT4, SHOOT5, SHOOT6, SHOOT7, SHOOT8, SHOOT9,

  PROWL_1, PROWL_2, PROWL_3, PROWL_4, PROWL_5, PROWL_6, PROWL_7, PROWL_8,
  PROWL_9, PROWL_10, PROWL_11, PROWL_12, PROWL_13, PROWL_14, PROWL_15, PROWL_16,
  PROWL_17, PROWL_18, PROWL_19, PROWL_20, PROWL_21, PROWL_22, PROWL_23, PROWL_24
};

enum army_states_e {
  MSTATE_DIE_A = MSTATE_EXTRA,
  MSTATE_DIE_B,
  MSTATE_PAIN_A,
  MSTATE_PAIN_B,
  MSTATE_PAIN_C,
};

static void army_stand(edict_t *self);
static void army_walk(edict_t *self);
static void army_run(edict_t *self);
static void army_missile(edict_t *self);
static void army_pain_a(edict_t *self);
static void army_pain_b(edict_t *self);
static void army_pain_c(edict_t *self);
static void army_die_a(edict_t *self);
static void army_die_b(edict_t *self);

static void army_start_die(edict_t *self, edict_t *killer);
static void army_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean army_check_attack(edict_t *self);

static const monster_state_t monster_army_states[MSTATE_MAX] = {
  /* STAND   */ { army_stand,   STAND1,  STAND8   },
  /* WALK    */ { army_walk,    PROWL_1, PROWL_24 },
  /* RUN     */ { army_run,     RUN1,    RUN8     },
  /* MISSILE */ { army_missile, SHOOT1,  SHOOT9   },
  /* MELEE   */ { NULL,         -1,      -1       },
  /* DIE_A   */ { army_die_a,   DEATH1,  DEATH10  },
  /* DIE_B   */ { army_die_b,   DEATHC1, DEATHC11 },
  /* PAIN_A  */ { army_pain_a,  PAIN1,   PAIN6    },
  /* PAIN_B  */ { army_pain_b,  PAINB1,  PAINB14  },
  /* PAIN_C  */ { army_pain_c,  PAINC1,  PAINC13  },
};

static const monster_class_t monster_army_class = {
  .state_table = monster_army_states,
  .fn_start_pain = army_start_pain,
  .fn_start_die = army_start_die,
  .fn_check_attack = army_check_attack,
  .sight_sound = SFXID_SOLDIER_SIGHT1
};

static void army_stand(edict_t *self) {
  ai_stand(self);
  monster_loop_state(self, MSTATE_STAND);
}

static void army_walk(edict_t *self) {
  // distance to move each frame
  static const s8 walktable[] = {
    1, 1, 1, 1, 2, 3, 4, 4, 2, 2, 2, 1,
    0, 1, 1, 1, 3, 3, 3, 3, 2, 1, 1, 1
  };

  switch (self->v.frame) {
  case PROWL_1:
    if (xrand32() < (ONE / 5))
      Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_SOLDIER_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
  /* fallthrough */
  default:
    ai_walk(self, TO_FIX32(walktable[self->v.frame - PROWL_1]));
    break;
  }

  monster_loop_state(self, MSTATE_WALK);
}

static void army_run(edict_t *self) {
  // distance to move each frame
  static const s8 runtable[] = {
    11, 15, 10, 10, 8, 15, 10, 8
  };

  switch (self->v.frame) {
  case RUN1:
    if (xrand32() < (ONE / 5))
      Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_SOLDIER_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
  /* fallthrough */
  default:
    ai_run(self, TO_FIX32(runtable[self->v.frame - RUN1]));
    break;
  }

  monster_loop_state(self, MSTATE_RUN);
}

static void army_fire(edict_t *self) {
  ai_face(self);

  Snd_StartSoundId(EDICT_NUM(self), CHAN_WEAPON, SFXID_WEAPONS_GUNCOCK, &self->v.origin, SND_MAXVOL, ATTN_NORM);
  self->v.effects |= EF_MUZZLEFLASH;

  // fire somewhat behind the player, so a dodging player is harder to hit
  edict_t *en = self->v.monster->enemy;
  x16vec3_t dir;
  x32 sqrlen;
  const x32vec3_t aim = {{
    en->v.origin.x - en->v.velocity.x / 5 - self->v.origin.x,
    en->v.origin.y - en->v.velocity.y / 5 - self->v.origin.y,
    en->v.origin.z - en->v.velocity.z / 5 - self->v.origin.z
  }};
  XVecNormLS(&aim, &dir, &sqrlen);

  utl_makevectors(&self->v.angles);
  utl_firebullets(self, 4, &dir, FTOX(0.1), FTOX(0.1));
}

static void army_missile(edict_t *self) {
  ai_face(self);

  switch (self->v.frame) {
  case SHOOT5: army_fire(self); break;
  case SHOOT7: ai_checkrefire(self, MSTATE_MISSILE); break;
  default: break;
  }

  monster_end_state(self, MSTATE_MISSILE, MSTATE_RUN);
}

static void army_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  monster_fields_t *mon = self->v.monster;

  if (mon->pain_finished > gs.time)
    return;

  const x32 r = xrand32();
  if (r < FTOX(0.2)) {
    mon->pain_finished = gs.time + FTOX(0.6);
    monster_set_state(self, MSTATE_PAIN_A);
  } else if (r < FTOX(0.6)) {
    mon->pain_finished = gs.time + FTOX(1.1);
    monster_set_state(self, MSTATE_PAIN_B);
  } else {
    mon->pain_finished = gs.time + FTOX(1.1);
    monster_set_state(self, MSTATE_PAIN_C);
  }

  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_SOLDIER_PAIN2, &self->v.origin, SND_MAXVOL, ATTN_NORM);
}

static void army_pain_a(edict_t *self) {
  if (self->v.frame == PAIN6)
    ai_pain(self, TO_FIX32(1));
  monster_end_state(self, MSTATE_PAIN_A, MSTATE_RUN);
}

static void army_pain_b(edict_t *self) {
  switch (self->v.frame) {
  case PAINB2:  ai_painforward(self, TO_FIX32(13)); break;
  case PAINB3:  ai_painforward(self, TO_FIX32(9)); break;
  case PAINB12: ai_pain(self, TO_FIX32(2)); break;
  default: break;
  }
  monster_end_state(self, MSTATE_PAIN_B, MSTATE_RUN);
}

static void army_pain_c(edict_t *self) {
  switch (self->v.frame) {
  case PAINC2:  ai_pain(self, TO_FIX32(1)); break;
  case PAINC5:  ai_painforward(self, TO_FIX32(1)); break;
  case PAINC6:  ai_painforward(self, TO_FIX32(1)); break;
  case PAINC8:  ai_pain(self, TO_FIX32(1)); break;
  case PAINC9:  ai_painforward(self, TO_FIX32(4)); break;
  case PAINC10: ai_painforward(self, TO_FIX32(3)); break;
  case PAINC11: ai_painforward(self, TO_FIX32(6)); break;
  case PAINC12: ai_painforward(self, TO_FIX32(5)); break;
  default: break;
  }
  monster_end_state(self, MSTATE_PAIN_C, MSTATE_RUN);
}

static inline void army_finish_die(edict_t *self) {
  self->v.solid = SOLID_NOT;
  utl_spawn_backpack(self, 0, AMMO_SHELLS, 5);
}

static void army_start_die(edict_t *self, edict_t *killer) {
  // check for gib
  if (self->v.health < -35) {
    ai_gib(self);
    return;
  }

  // regular death
  Snd_StartSoundId(EDICT_NUM(self), CHAN_VOICE, SFXID_SOLDIER_DEATH1, &self->v.origin, SND_MAXVOL, ATTN_NORM);

  monster_set_state(self, (xrand32() < HALF) ? MSTATE_DIE_A : MSTATE_DIE_B);
}

static void army_die_a(edict_t *self) {
  if (self->v.frame == DEATH3)
    army_finish_die(self);
  monster_end_state(self, MSTATE_DIE_A, -1);
}

static void army_die_b(edict_t *self) {
  switch (self->v.frame) {
  case DEATHC2: ai_back(self, TO_FIX32(5)); break;
  case DEATHC3:
    army_finish_die(self);
    ai_back(self, TO_FIX32(4));
    break;
  case DEATHC4: ai_back(self, TO_FIX32(13)); break;
  case DEATHC5: ai_back(self, TO_FIX32(3)); break;
  case DEATHC6: ai_back(self, TO_FIX32(4)); break;
  default: break;
  }
  monster_end_state(self, MSTATE_DIE_B, -1);
}

static qboolean army_check_attack(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  edict_t *targ = self->v.monster->enemy;

  // see if any entities are in the way of the shot
  x32vec3_t spot1 = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + self->v.viewheight }};
  x32vec3_t spot2 = {{ targ->v.origin.x, targ->v.origin.y, targ->v.origin.z + targ->v.viewheight }};
  const trace_t *trace = utl_traceline(&spot1, &spot2, false, self);
  if (trace->ent != targ)
    return false; // no clear shot
  if (trace->inopen && trace->inwater)
    return false; // sight line crossed contents

  if (pr.enemy_range == RANGE_FAR || mon->attack_finished > gs.time)
    return false;

  x32 chance;
  switch (pr.enemy_range) {
  case RANGE_MELEE: chance = FTOX(0.9); break;
  case RANGE_NEAR:  chance = FTOX(0.4); break;
  case RANGE_MID:   chance = FTOX(0.05); break;
  default:
    chance = 0;
    break;
  }

  if (xrand32() < chance) {
    monster_set_state(self, MSTATE_MISSILE);
    ai_attack_finished(self, ONE + xrand32());
    if (xrand32() < FTOX(0.3))
      mon->lefty = !mon->lefty;
    return true;
  }

  return false;
}

void spawn_monster_army(edict_t *self) {
  G_SetModel(self, MDLID_SOLDIER);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +24);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.health = self->v.max_health = 30;
  walkmonster_start(self, &monster_army_class);
}
