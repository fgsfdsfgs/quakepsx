#include "prcommon.h"

#define SF_SPAWN_CRUCIFIED 1

enum zombie_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8,
  STAND9, STAND10, STAND11, STAND12, STAND13, STAND14, STAND15,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7, WALK8, WALK9, WALK10, WALK11,
  WALK12, WALK13, WALK14, WALK15, WALK16, WALK17, WALK18, WALK19,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6, RUN7, RUN8, RUN9, RUN10, RUN11, RUN12,
  RUN13, RUN14, RUN15, RUN16, RUN17, RUN18,

  ATTA1, ATTA2, ATTA3, ATTA4, ATTA5, ATTA6, ATTA7, ATTA8, ATTA9, ATTA10, ATTA11,
  ATTA12, ATTA13,

  ATTB1, ATTB2, ATTB3, ATTB4, ATTB5, ATTB6, ATTB7, ATTB8, ATTB9, ATTB10, ATTB11,
  ATTB12, ATTB13, ATTB14,

  ATTC1, ATTC2, ATTC3, ATTC4, ATTC5, ATTC6, ATTC7, ATTC8, ATTC9, ATTC10, ATTC11,
  ATTC12,

  PAINA1, PAINA2, PAINA3, PAINA4, PAINA5, PAINA6, PAINA7, PAINA8, PAINA9, PAINA10,
  PAINA11, PAINA12,

  PAINB1, PAINB2, PAINB3, PAINB4, PAINB5, PAINB6, PAINB7, PAINB8, PAINB9, PAINB10,
  PAINB11, PAINB12, PAINB13, PAINB14, PAINB15, PAINB16, PAINB17, PAINB18, PAINB19,
  PAINB20, PAINB21, PAINB22, PAINB23, PAINB24, PAINB25, PAINB26, PAINB27, PAINB28,

  PAINC1, PAINC2, PAINC3, PAINC4, PAINC5, PAINC6, PAINC7, PAINC8, PAINC9, PAINC10,
  PAINC11, PAINC12, PAINC13, PAINC14, PAINC15, PAINC16, PAINC17, PAINC18,

  PAIND1, PAIND2, PAIND3, PAIND4, PAIND5, PAIND6, PAIND7, PAIND8, PAIND9, PAIND10,
  PAIND11, PAIND12, PAIND13,

  PAINE1, PAINE2, PAINE3, PAINE4, PAINE5, PAINE6, PAINE7, PAINE8, PAINE9, PAINE10,
  PAINE11, PAINE12, PAINE13, PAINE14, PAINE15, PAINE16, PAINE17, PAINE18, PAINE19,
  PAINE20, PAINE21, PAINE22, PAINE23, PAINE24, PAINE25, PAINE26, PAINE27, PAINE28,
  PAINE29, PAINE30,

  CRUC_1, CRUC_2, CRUC_3, CRUC_4, CRUC_5, CRUC_6
};

enum zombie_states_e {
  MSTATE_ATTACK_B = MSTATE_EXTRA,
  MSTATE_ATTACK_C,
  MSTATE_CRUC,
  MSTATE_PAIN_A,
  MSTATE_PAIN_B,
  MSTATE_PAIN_C,
  MSTATE_PAIN_D,
  MSTATE_PAIN_E,
};

static void zombie_stand(edict_t *self);
static void zombie_walk(edict_t *self);
static void zombie_run(edict_t *self);
static void zombie_attk_a(edict_t *self);
static void zombie_attk_b(edict_t *self);
static void zombie_attk_c(edict_t *self);
static void zombie_pain_a(edict_t *self);
static void zombie_pain_b(edict_t *self);
static void zombie_pain_c(edict_t *self);
static void zombie_pain_d(edict_t *self);
static void zombie_pain_e(edict_t *self);
static void zombie_cruc(edict_t *self);

static void zombie_start_die(edict_t *self, edict_t *killer);
static void zombie_start_pain(edict_t *self, edict_t *attacker, s16 damage);

static const monster_state_t monster_zombie_states[MSTATE_MAX] = {
  /* STAND    */ { zombie_stand,   STAND1, STAND8  },
  /* WALK     */ { zombie_walk,    WALK1,  WALK19  },
  /* RUN      */ { zombie_run,     RUN1,   RUN18   },
  /* MISSILE  */ { zombie_attk_a,  ATTA1,  ATTA13  },
  /* MELEE    */ { NULL,           -1,     -1      },
  /* ATTACK_B */ { zombie_attk_b,  ATTB1,  ATTB14  },
  /* ATTACK_C */ { zombie_attk_c,  ATTC1,  ATTC12  },
  /* CRUC     */ { zombie_cruc,    CRUC_1, CRUC_6  },
  /* PAIN_A   */ { zombie_pain_a,  PAINA1, PAINA12 },
  /* PAIN_B   */ { zombie_pain_b,  PAINB1, PAINB28 },
  /* PAIN_C   */ { zombie_pain_c,  PAINC1, PAINC18 },
  /* PAIN_D   */ { zombie_pain_d,  PAIND1, PAIND13 },
  /* PAIN_E   */ { zombie_pain_e,  PAINE1, PAINE30 },
};

static const monster_class_t monster_zombie_class = {
  .state_table = monster_zombie_states,
  .fn_start_pain = zombie_start_pain,
  .fn_start_die = zombie_start_die,
  .fn_check_attack = NULL,
  .sight_sound = 0
};

static void zombie_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  ai_stand(self);
}

static void zombie_walk(edict_t *self) {
  monster_looping_state(self, MSTATE_WALK);

  x32 d;
  switch (self->v.frame) {
  case WALK2:  d = TO_FIX32(2); break;
  case WALK3:  d = TO_FIX32(3); break;
  case WALK4:  d = TO_FIX32(2); break;
  case WALK5:  d = TO_FIX32(1); break;
  case WALK11: d = TO_FIX32(2); break;
  case WALK12: d = TO_FIX32(2); break;
  case WALK13: d = TO_FIX32(1); break;
  case WALK19:
    if (xrand32() < FTOX(0.2))
      utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_IDLE, SND_MAXVOL, ATTN_IDLE);
    /* fallthrough */
  default:
    d = 0;
    break;
  }

  ai_walk(self, d);
}

static void zombie_run(edict_t *self) {
  static const s8 runtable[] = {
    1, 1, 0, 1, 2, 3, 4, 4, 2, 0, 0, 0,
    2, 4, 6, 7, 3, 8
  };

  monster_looping_state(self, MSTATE_RUN);

  ai_run(self, (x32)runtable[self->v.frame - RUN1] << FIXSHIFT);

  if (self->v.frame == RUN18 && xrand32() < FTOX(0.2))
    utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_IDLE, SND_MAXVOL, ATTN_IDLE);
}

static void zombie_cruc(edict_t *self) {
  monster_looping_state(self, MSTATE_CRUC);

  if (self->v.frame == CRUC_1 && xrand32() < FTOX(0.1))
    utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_IDLE_W2, SND_MAXVOL, ATTN_IDLE);

  self->v.nextthink = gs.time + PR_FRAMETIME + (rand() & 255);
}

static void zombie_grenade_remove_touch(edict_t *self, edict_t *other) {
  utl_remove(self);
}

static void zombie_grenade_touch(edict_t *self, edict_t *other) {
  if (other == self->v.owner)
    return;

  utl_sound(self, CHAN_WEAPON, SFXID_ZOMBIE_Z_MISS, SND_MAXVOL, ATTN_NORM);

  if (other->v.flags & FL_TAKEDAMAGE) {
    utl_damage(other, self, self->v.owner, 10);
    utl_remove(self);
    return;
  }

  XVecZero(&self->v.velocity);
  XVecZero(&self->v.avelocity);
  self->v.touch = zombie_grenade_remove_touch;
}

static void zombie_fire_grenade(edict_t *self, const s16vec3_t *st) {
  utl_sound(self, CHAN_WEAPON, SFXID_ZOMBIE_Z_SHOT1, SND_MAXVOL, ATTN_NORM);

  edict_t *missile = ED_Alloc();
  missile->v.owner = self;
  missile->v.movetype = MOVETYPE_BOUNCE;
  missile->v.solid = SOLID_BBOX;
  missile->v.touch = zombie_grenade_touch;
  missile->v.think = utl_remove;
  missile->v.nextthink = gs.time + FTOX(2.5);
  missile->v.effects |= EF_GIB;

  utl_makevectors(&self->v.angles);
  const s16 sz = st->z - 24;
  missile->v.origin.x = self->v.origin.x + st->x * pr.v_forward.x + sz * pr.v_up.x;
  missile->v.origin.y = self->v.origin.y + st->y * pr.v_forward.y + sz * pr.v_up.y;
  missile->v.origin.z = self->v.origin.z + st->z * pr.v_forward.z + sz * pr.v_up.z;

  x32vec3_t delta;
  x16vec3_t dir;
  x32 xysqrlen;
  XVecSub(&self->v.monster->enemy->v.origin, &self->v.origin, &delta);
  XVecNormLS(&delta, &dir, &xysqrlen);

  missile->v.velocity.x = 600 * (x32)dir.x;
  missile->v.velocity.y = 600 * (x32)dir.y;
  missile->v.velocity.z = TO_FIX32(200);

  XVecSet(&missile->v.avelocity, TO_DEG16(960), TO_DEG16(360), TO_DEG16(720));

  G_SetModel(missile, MDLID_ZOM_GIB);
  G_SetSize(missile, &x32vec3_origin, &x32vec3_origin);
  G_LinkEdict(missile, false);
}

static void zombie_attk_a(edict_t *self) {
  if (self->v.monster->next_frame == ATTA1) {
    // randomly chain into one of the alternate attacks instead
    const x32 r = xrand32();
    if (r >= FTOX(0.3) && r < FTOX(0.6)) {
      monster_exec_state(self, MSTATE_ATTACK_B);
      return;
    } else if (r >= FTOX(0.6)) {
      monster_exec_state(self, MSTATE_ATTACK_C);
      return;
    }
  }

  monster_finite_state(self, MSTATE_MISSILE, MSTATE_RUN);

  ai_face(self);

  s16vec3_t d;
  if (self->v.frame == ATTA13) {
    XVecSet(&d, -10, -22, 30);
    zombie_fire_grenade(self, &d);
  }
}

static void zombie_attk_b(edict_t *self) {
  monster_finite_state(self, MSTATE_ATTACK_B, MSTATE_RUN);

  ai_face(self);

  s16vec3_t d;
  if (self->v.frame == ATTB13) {
    XVecSet(&d, -10, -24, 29);
    zombie_fire_grenade(self, &d);
  }
}

static void zombie_attk_c(edict_t *self) {
  monster_finite_state(self, MSTATE_ATTACK_C, MSTATE_RUN);

  ai_face(self);

  s16vec3_t d;
  if (self->v.frame == ATTC12) {
    XVecSet(&d, -12, -19, 29);
    zombie_fire_grenade(self, &d);
  }
}

static void zombie_pain_a(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_A, MSTATE_RUN);

  switch (self->v.frame) {
  case PAINA2: ai_painforward(self, TO_FIX32(3)); break;
  case PAINA3: ai_painforward(self, TO_FIX32(1)); break;
  case PAINA5: ai_pain(self, TO_FIX32(1)); break;
  case PAINA6: ai_pain(self, TO_FIX32(3)); break;
  case PAINA7: ai_pain(self, TO_FIX32(1)); break;
  default: break;
  }
}

static void zombie_pain_b(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_B, MSTATE_RUN);

  switch (self->v.frame) {
  case PAINB2:  ai_pain(self, TO_FIX32(2)); break;
  case PAINB3:  ai_pain(self, TO_FIX32(8)); break;
  case PAINB4:  ai_pain(self, TO_FIX32(6)); break;
  case PAINB5:  ai_pain(self, TO_FIX32(2)); break;
  case PAINB9:  utl_sound(self, CHAN_BODY, SFXID_ZOMBIE_Z_FALL, SND_MAXVOL, ATTN_NORM); break;
  case PAINB25: ai_painforward(self, ONE); break;
  default: break;
  }
}

static void zombie_pain_c(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_C, MSTATE_RUN);

  switch (self->v.frame) {
  case PAINC3:  ai_pain(self, TO_FIX32(3)); break;
  case PAINC4:  ai_pain(self, TO_FIX32(1)); break;
  case PAINC11:
  case PAINC12: ai_painforward(self, ONE); break;
  default: break;
  }
}

static void zombie_pain_d(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_D, MSTATE_RUN);
  if (self->v.frame == PAIND9)
    ai_pain(self, ONE);
}

static void zombie_pain_e(edict_t *self) {
  static const s8 paintable[] = {
    8, 5, 3, 1, 2, 1, 1, 2
  };

  monster_finite_state(self, MSTATE_PAIN_E, MSTATE_RUN);

  switch (self->v.frame) {
  case PAINE1:
    self->v.health = self->v.max_health;
    utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_PAIN, SND_MAXVOL, ATTN_NORM);
    break;
  case PAINE10:
    self->v.solid = SOLID_NOT;
    utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_FALL, SND_MAXVOL, ATTN_NORM);
    break;
  case PAINE11:
    self->v.nextthink = gs.time + TO_FIX32(5);
    self->v.health = self->v.max_health;
    break;
  case PAINE12:
    // see if ok to stand up
    utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_IDLE, SND_MAXVOL, ATTN_IDLE);
    self->v.health = self->v.max_health;
    self->v.solid = SOLID_SLIDEBOX;
    if (!ai_walkmove(self, 0, 0)) {
      self->v.monster->next_frame = PAINE11;
      self->v.solid = SOLID_NOT;
    }
    break;
  case PAINE2 ... PAINE9:
    ai_pain(self, TO_FIX32(paintable[self->v.frame - PAINE2]));
    break;
  case PAINE25:
    ai_painforward(self, TO_FIX32(5));
    break;
  case PAINE26:
    ai_painforward(self, TO_FIX32(3));
    break;
  case PAINE27:
    ai_painforward(self, TO_FIX32(1));
    break;
  case PAINE28:
    ai_pain(self, TO_FIX32(1));
    break;
  default:
    break;
  }
}

static void zombie_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  monster_fields_t *mon = self->v.monster;

  self->v.health = self->v.max_health; // always reset health

  if (damage < 9)
    return; // totally ignore

  if (mon->state_num == MSTATE_PAIN_E)
    return;  // down on ground, so don't reset any counters

  // go down immediately if a big enough hit
  if (damage >= 25) {
    monster_exec_state(self, MSTATE_PAIN_E);
    return;
  }

  if (mon->state_num >= MSTATE_PAIN_A) {
    // if hit again in next three seconds while not in pain frames, definitely drop
    mon->pain_finished = gs.time + TO_FIX32(3);
    return; // currently going through an animation, don't change
  }

  if (mon->pain_finished > gs.time) {
    // hit again, so drop down
    monster_exec_state(self, MSTATE_PAIN_E);
    return;
  }

  // go into one of the fast pain animations
  const x32 r = xrand32();
  s16 state;
  if (r < FTOX(0.25))
    state = MSTATE_PAIN_A;
  else if (r < FTOX(0.5))
    state = MSTATE_PAIN_B;
  else if (r < FTOX(0.75))
    state = MSTATE_PAIN_C;
  else
    state = MSTATE_PAIN_D;
  utl_sound(self, CHAN_VOICE, SFXID_ZOMBIE_Z_PAIN, SND_MAXVOL, ATTN_NORM);
  monster_exec_state(self, state);
}

static void zombie_start_die(edict_t *self, edict_t *killer) {
  ai_gib(self);
}

void spawn_monster_zombie(edict_t *self) {
  G_SetModel(self, MDLID_ZOMBIE);

  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);

  self->v.health = self->v.max_health = 60;

  if (self->v.spawnflags & SF_SPAWN_CRUCIFIED) {
    monster_init(self, &monster_zombie_class);
    pr.total_monsters--; // don't count crucified zombies
    self->v.movetype = MOVETYPE_NONE;
    monster_exec_state(self, MSTATE_CRUC);
  } else {
    walkmonster_start(self, &monster_zombie_class);
  }
}
