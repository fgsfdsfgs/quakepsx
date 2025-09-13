#include "prcommon.h"

enum ogre_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8, STAND9,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7,
  WALK8, WALK9, WALK10, WALK11, WALK12, WALK13, WALK14, WALK15, WALK16,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6, RUN7, RUN8,

  SWING1, SWING2, SWING3, SWING4, SWING5, SWING6, SWING7,
  SWING8, SWING9, SWING10, SWING11, SWING12, SWING13, SWING14,

  SMASH1, SMASH2, SMASH3, SMASH4, SMASH5, SMASH6, SMASH7,
  SMASH8, SMASH9, SMASH10, SMASH11, SMASH12, SMASH13, SMASH14,

  SHOOT1, SHOOT2, SHOOT3, SHOOT4, SHOOT5, SHOOT6,

  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5,

  PAINB1, PAINB2, PAINB3,

  PAINC1, PAINC2, PAINC3, PAINC4, PAINC5, PAINC6,

  PAIND1, PAIND2, PAIND3, PAIND4, PAIND5, PAIND6, PAIND7, PAIND8, PAIND9, PAIND10,
  PAIND11, PAIND12, PAIND13, PAIND14, PAIND15, PAIND16,

  PAINE1, PAINE2, PAINE3, PAINE4, PAINE5, PAINE6, PAINE7, PAINE8, PAINE9, PAINE10,
  PAINE11, PAINE12, PAINE13, PAINE14, PAINE15,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6,
  DEATH7, DEATH8, DEATH9, DEATH10, DEATH11, DEATH12,
  DEATH13, DEATH14,

  BDEATH1, BDEATH2, BDEATH3, BDEATH4, BDEATH5, BDEATH6,
  BDEATH7, BDEATH8, BDEATH9, BDEATH10,

  /* // unused
  PULL1, PULL2, PULL3, PULL4, PULL5, PULL6, PULL7, PULL8, PULL9, PULL10, PULL11
  */
};

enum ogre_states_e {
  MSTATE_SMASH = MSTATE_EXTRA,
  MSTATE_DIE_A,
  MSTATE_DIE_B,
  MSTATE_PAIN_A,
  MSTATE_PAIN_B,
  MSTATE_PAIN_C,
  MSTATE_PAIN_D,
  MSTATE_PAIN_E,
};

static void ogre_stand(edict_t *self);
static void ogre_walk(edict_t *self);
static void ogre_run(edict_t *self);
static void ogre_missile(edict_t *self);
static void ogre_swing(edict_t *self);
static void ogre_smash(edict_t *self);
static void ogre_pain_a(edict_t *self);
static void ogre_pain_b(edict_t *self);
static void ogre_pain_c(edict_t *self);
static void ogre_pain_d(edict_t *self);
static void ogre_pain_e(edict_t *self);
static void ogre_die_a(edict_t *self);
static void ogre_die_b(edict_t *self);

static void ogre_start_die(edict_t *self, edict_t *killer);
static void ogre_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean ogre_check_attack(edict_t *self);

static const monster_state_t monster_ogre_states[MSTATE_MAX] = {
  /* STAND   */ { ogre_stand,   STAND1,  STAND9   },
  /* WALK    */ { ogre_walk,    WALK1,   WALK16   },
  /* RUN     */ { ogre_run,     RUN1,    RUN8     },
  /* MISSILE */ { ogre_missile, SHOOT1,  SHOOT6   },
  /* MELEE   */ { ogre_swing,   SWING1,  SWING14  },
  /* SMASH   */ { ogre_smash,   SMASH1,  SMASH14  },
  /* DIE_A   */ { ogre_die_a,   DEATH1,  DEATH14  },
  /* DIE_B   */ { ogre_die_b,   BDEATH1, BDEATH10 },
  /* PAIN_A  */ { ogre_pain_a,  PAIN1,   PAIN5    },
  /* PAIN_B  */ { ogre_pain_b,  PAINB1,  PAINB3   },
  /* PAIN_C  */ { ogre_pain_c,  PAINC1,  PAINC6   },
  /* PAIN_D  */ { ogre_pain_d,  PAIND1,  PAIND16  },
  /* PAIN_E  */ { ogre_pain_e,  PAINE1,  PAINE15  },
};

static const monster_class_t monster_ogre_class = {
  .state_table = monster_ogre_states,
  .fn_start_pain = ogre_start_pain,
  .fn_start_die = ogre_start_die,
  .fn_check_attack = ogre_check_attack,
  .sight_sound = SFXID_OGRE_OGWAKE,
};

static void ogre_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  if (self->v.frame == STAND5 && xrand32() < FTOX(0.1))
    utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGIDLE, SND_MAXVOL, ATTN_IDLE);
  ai_stand(self);
}

static void ogre_walk(edict_t *self) {
  // distance to move each frame
  static const s8 walktable[] = {
    3, 2, 2, 2, 2, 5, 3, 2, 3, 1, 2, 3,
    3, 3, 3, 4
  };

  monster_looping_state(self, MSTATE_WALK);

  switch (self->v.frame) {
  case WALK3:
    if (xrand32() < FTOX(0.1))
      utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGIDLE, SND_MAXVOL, ATTN_IDLE);
    break;
  case WALK6:
    if (xrand32() < FTOX(0.1))
      utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGDRAG, SND_MAXVOL, ATTN_IDLE);
    break;
  default:
    break;
  }

  ai_walk(self, TO_FIX32(walktable[self->v.frame - WALK1]));
}

static void ogre_run(edict_t *self) {
  monster_looping_state(self, MSTATE_RUN);

  // distance to move each frame
  static const s8 runtable[] = {
    9, 12, 8, 22, 16, 4, 13, 24
  };

  switch (self->v.frame) {
  case RUN8:
    if (xrand32() < FTOX(0.1))
      utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGIDLE2, SND_MAXVOL, ATTN_IDLE);
  /* fallthrough */
  default:
    ai_run(self, TO_FIX32(runtable[self->v.frame - RUN1]));
    break;
  }
}

static void ogre_grenade(edict_t *self) {
  self->v.effects |= EF_MUZZLEFLASH;

  utl_sound(self, CHAN_WEAPON, SFXID_WEAPONS_GRENADE, SND_MAXVOL, ATTN_NORM);

  edict_t *missile = utl_launch_grenade(self, &self->v.angles);
  missile->v.dmg = 40;

  // set missile speed
  x32vec3_t delta;
  x16vec3_t dir;
  x32 xysqrdist;
  XVecSub(&self->v.monster->enemy->v.origin, &self->v.origin, &delta);
  XVecNormLS(&delta, &dir, &xysqrdist);
  missile->v.velocity.x = 600 * (x32)dir.x;
  missile->v.velocity.y = 600 * (x32)dir.y;
  missile->v.velocity.z = TO_FIX32(200);
}

static void ogre_missile(edict_t *self) {
  monster_finite_state(self, MSTATE_MISSILE, MSTATE_RUN);

  ai_face(self);

  if (self->v.frame == SHOOT3)
    ogre_grenade(self);
}

static void ogre_chainsaw(edict_t *self, const x32 side) {
  monster_fields_t *mon = self->v.monster;

  if (!mon->enemy)
    return;
  if (!utl_can_damage(self, mon->enemy, self))
    return;

  ai_charge(self, TO_FIX32(10));

  x32vec3_t delta;
  XVecSub(&mon->enemy->v.origin, &self->v.origin, &delta);
  if (XVecLengthSqrIntL(&delta) > 100 * 100)
    return;

  const s16 ldmg = xmul32(xrand32() + xrand32() + xrand32(),  4);
  utl_damage(mon->enemy, self, self, ldmg);

  if (side) {
    const x32vec3_t org = {{
      self->v.origin.x + (delta.x >> 1),
      self->v.origin.y + (delta.y >> 1),
      self->v.origin.z + (delta.z >> 1)
    }};
    fx_spawn_blood(&org, ldmg);
  }
}

static void ogre_swing(edict_t *self) {
  static const s16 sawtable[] = {
    0, 200, 0, 0, 0, -200, 0
  };

  // if we're transitioning into the melee state, randomly choose the alt melee
  if (self->v.monster->next_frame == SWING1) {
    if (xrand32() >= FTOX(0.5)) {
      monster_exec_state(self, MSTATE_SMASH);
      return;
    }
  }

  monster_finite_state(self, MSTATE_MELEE, MSTATE_RUN);

  switch (self->v.frame) {
  case SWING1:
    ai_charge(self, TO_FIX32(11));
    utl_sound(self, CHAN_WEAPON, SFXID_OGRE_OGSAWATK, SND_MAXVOL, ATTN_NORM);
    break;
  case SWING2: ai_charge(self, TO_FIX32(1)); break;
  case SWING3: ai_charge(self, TO_FIX32(4)); break;
  case SWING4: ai_charge(self, TO_FIX32(13)); break;
  case SWING5:
    ai_charge(self, TO_FIX32(9));
    /* fallthrough */
  case SWING6:
  case SWING7:
  case SWING8:
  case SWING9:
  case SWING10:
  case SWING11:
    ogre_chainsaw(self, TO_FIX32(sawtable[self->v.frame - SWING5]));
    self->v.angles.y += XMUL16(xrand32(), TO_DEG16(25));
    break;
  case SWING12: ai_charge(self, TO_FIX32(3)); break;
  case SWING13: ai_charge(self, TO_FIX32(8)); break;
  case SWING14: ai_charge(self, TO_FIX32(9)); break;
  default: break;
  }
}

static void ogre_smash(edict_t *self) {
  static const s8 chargetable[] = {
    6, 0, 0, 1, 4, 4, 4, 10, 13, -1, 2,
    0, 4, 12
  };

  monster_finite_state(self, MSTATE_SMASH, MSTATE_RUN);

  const s16 dframe = self->v.frame - SMASH1;
  if (chargetable[dframe] >= 0)
    ai_charge(self, TO_FIX32(chargetable[dframe]));

  switch (self->v.frame) {
  case SMASH1:
    utl_sound(self, CHAN_WEAPON, SFXID_OGRE_OGSAWATK, SND_MAXVOL, ATTN_NORM);
    break;
  case SMASH6:
  case SMASH7:
  case SMASH8:
  case SMASH9:
  case SMASH12:
    ogre_chainsaw(self, 0);
    break;
  case SMASH11:
    ogre_chainsaw(self, 1);
    break;
  default:
    break;
  }
}

static void ogre_pain_a(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_A, MSTATE_RUN);
}

static void ogre_pain_b(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_B, MSTATE_RUN);
}

static void ogre_pain_c(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_C, MSTATE_RUN);
}

static void ogre_pain_d(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_D, MSTATE_RUN);
  switch (self->v.frame) {
  case PAIND2: ai_pain(self, TO_FIX32(10)); break;
  case PAIND3: ai_pain(self, TO_FIX32(9)); break;
  case PAIND4: ai_pain(self, TO_FIX32(4)); break;
  default: break;
  }
}

static void ogre_pain_e(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_E, MSTATE_RUN);
  switch (self->v.frame) {
  case PAINE2: ai_pain(self, TO_FIX32(10)); break;
  case PAINE3: ai_pain(self, TO_FIX32(9)); break;
  case PAINE4: ai_pain(self, TO_FIX32(4)); break;
  default: break;
  }
}

static void ogre_finish_die(edict_t *self) {
  self->v.solid = SOLID_NOT;
  utl_spawn_backpack(self, 0, AMMO_ROCKETS, 2);
}

static void ogre_die_a(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE_A, -1);
  if (self->v.frame == DEATH3)
    ogre_finish_die(self);
  else if (self->v.frame == DEATH14)
    ai_fade_corpse(self);
}

static void ogre_die_b(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE_B, -1);
  switch (self->v.frame) {
  case BDEATH2: ai_forward(self, TO_FIX32(5)); break;
  case BDEATH3: ogre_finish_die(self); break;
  case BDEATH4: ai_forward(self, TO_FIX32(1)); break;
  case BDEATH5: ai_forward(self, TO_FIX32(3)); break;
  case BDEATH6: ai_forward(self, TO_FIX32(7)); break;
  case BDEATH7: ai_forward(self, TO_FIX32(25)); break;
  case BDEATH10: ai_fade_corpse(self); break;
  default: break;
  }
}

static void ogre_start_die(edict_t *self, edict_t *killer) {
  // check for gib
  if (self->v.health < -80) {
    ai_gib(self);
    return;
  }
  utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGDTH, SND_MAXVOL, ATTN_NORM);
  monster_exec_state(self, (rand() & 1) + MSTATE_DIE_A);
}

static void ogre_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  monster_fields_t *mon = self->v.monster;

  // don't make multiple pain sounds right after each other
  if (mon->pain_finished > gs.time)
    return;

  utl_sound(self, CHAN_VOICE, SFXID_OGRE_OGPAIN1, SND_MAXVOL, ATTN_NORM);

  const x32 r = xrand32();
  s16 pain_state;
  x32 pain_duration;
  if (r < FTOX(0.25)) {
    pain_state = MSTATE_PAIN_A;
    pain_duration = ONE;
  } else if (r < FTOX(0.5)) {
    pain_state = MSTATE_PAIN_B;
    pain_duration = ONE;
  } else if (r < FTOX(0.75)) {
    pain_state = MSTATE_PAIN_C;
    pain_duration = ONE;
  } else if (r < FTOX(0.88)) {
    pain_state = MSTATE_PAIN_D;
    pain_duration = TO_FIX32(2);
  } else {
    pain_state = MSTATE_PAIN_E;
    pain_duration = TO_FIX32(2);
  }

  mon->pain_finished = gs.time + pain_duration;
  monster_exec_state(self, pain_state);
}

static qboolean ogre_check_attack(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  if (pr.enemy_range == RANGE_MELEE) {
    if (utl_can_damage(self, mon->enemy, self)) {
      mon->attack_state = AS_MELEE;
      return true;
    }
  }

  if (gs.time < mon->attack_finished)
    return false;

  if (!pr.enemy_vis)
    return false;

  edict_t *targ = mon->enemy;

  // see if any entities are in the way of the shot
  const x32vec3_t spot1 = {{
    self->v.origin.x,
    self->v.origin.y,
    self->v.origin.z + self->v.viewheight
  }};
  const x32vec3_t spot2 = {{
    targ->v.origin.x,
    targ->v.origin.y,
    targ->v.origin.z + targ->v.viewheight
  }};
  x32vec3_t delta;
  XVecSub(&spot1, &spot2, &delta);

  if (XVecLengthSqrIntL(&delta) > 600 * 600)
    return false;

  const trace_t *trace = utl_traceline(&spot1, &spot2, false, self);
  if (trace->inopen && trace->inwater)
    return false; // sight line crossed contents
  if (trace->ent != targ)
    return false; // don't have a clear shot

  // missile attack
  if (pr.enemy_range == RANGE_FAR)
    return false;

  mon->attack_state = AS_MISSILE;
  ai_attack_finished(self, TO_FIX32(2) + 2 * xrand32());
  return true;
}

void spawn_monster_ogre(edict_t *self) {
  G_SetModel(self, MDLID_OGRE);
  G_SetSize(self, &gs.worldmodel->hulls[2].mins, &gs.worldmodel->hulls[2].maxs);
  self->v.health = self->v.max_health = 200;
  walkmonster_start(self, &monster_ogre_class);
}
