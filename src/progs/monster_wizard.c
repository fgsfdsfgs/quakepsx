#include "prcommon.h"

enum wizard_frames_e {
  HOVER1, HOVER2, HOVER3, HOVER4, HOVER5, HOVER6, HOVER7, HOVER8,
  HOVER9, HOVER10, HOVER11, HOVER12, HOVER13, HOVER14, HOVER15,

  FLY1, FLY2, FLY3, FLY4, FLY5, FLY6, FLY7, FLY8, FLY9, FLY10,
  FLY11, FLY12, FLY13, FLY14,

  MAGATT1, MAGATT2, MAGATT3, MAGATT4, MAGATT5, MAGATT6, MAGATT7,
  MAGATT8, MAGATT9, MAGATT10, MAGATT11, MAGATT12, MAGATT13,

  PAIN1, PAIN2, PAIN3, PAIN4,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8
};

enum wizard_states_e {
  MSTATE_SIDE = MSTATE_EXTRA,
  MSTATE_DIE,
  MSTATE_PAIN,
};

static void wizard_stand(edict_t *self);
static void wizard_walk(edict_t *self);
static void wizard_run(edict_t *self);
static void wizard_side(edict_t *self);
static void wizard_missile(edict_t *self);
static void wizard_pain(edict_t *self);
static void wizard_die(edict_t *self);

static void wizard_start_die(edict_t *self, edict_t *killer);
static void wizard_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean wizard_check_attack(edict_t *self);

static const monster_state_t monster_wizard_states[MSTATE_MAX] = {
  /* STAND   */ { wizard_stand,   HOVER1,  HOVER8   },
  /* WALK    */ { wizard_walk,    HOVER1,  HOVER8   },
  /* RUN     */ { wizard_run,     FLY1,    FLY14    },
  /* MISSILE */ { wizard_missile, MAGATT1, MAGATT10 },
  /* MELEE   */ { NULL,           -1,      -1       },
  /* SIDE    */ { wizard_side,    HOVER1,  HOVER8   },
  /* DIE     */ { wizard_die,     DEATH1,  DEATH8   },
  /* PAIN    */ { wizard_pain,    PAIN1,   PAIN4    },
};

static const monster_class_t monster_wizard_class = {
  .state_table = monster_wizard_states,
  .fn_start_pain = wizard_start_pain,
  .fn_start_die = wizard_start_die,
  .fn_check_attack = wizard_check_attack,
  .sight_sound = SFXID_WIZARD_WSIGHT
};

static void wizard_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  ai_stand(self);
}

static void wizard_walk(edict_t *self) {
  monster_looping_state(self, MSTATE_WALK);
  ai_walk(self, TO_FIX32(8));
  if (self->v.frame == HOVER1 && (xrand32() < FTOX(0.05)))
    utl_sound(self, CHAN_VOICE, SFXID_WIZARD_WIDLE1, SND_MAXVOL, ATTN_IDLE);
}

static void wizard_side(edict_t *self) {
  monster_looping_state(self, MSTATE_SIDE);
  ai_run(self, TO_FIX32(8));
  if (self->v.frame == HOVER1 && (xrand32() < FTOX(0.05)))
    utl_sound(self, CHAN_VOICE, SFXID_WIZARD_WIDLE1, SND_MAXVOL, ATTN_IDLE);
}

static void wizard_run(edict_t *self) {
  monster_looping_state(self, MSTATE_RUN);
  ai_run(self, TO_FIX32(16));
  if (self->v.frame == FLY14 && (xrand32() < FTOX(0.05)))
    utl_sound(self, CHAN_VOICE, SFXID_WIZARD_WIDLE1, SND_MAXVOL, ATTN_IDLE);
}

static void wizard_missile_start(edict_t *self) {
  if (self->v.owner->v.health <= 0) {
    utl_remove(self);
    return;
  }

  self->v.owner->v.effects |= EF_MUZZLEFLASH;

  edict_t *enemy = self->v.extra_ptr;

  const x32vec3_t dst = {{
    enemy->v.origin.x - 13 * self->v.avelocity.x,
    enemy->v.origin.y - 13 * self->v.avelocity.y,
    enemy->v.origin.z - 13 * self->v.avelocity.z,
  }};

  x32vec3_t delta;
  x16vec3_t dir;
  x32 xysqrlen;
  XVecSub(&dst, &self->v.origin, &delta);
  XVecNormLS(&delta, &dir, &xysqrlen);

  utl_vectoangles(&dir);

  utl_sound(self, CHAN_WEAPON, SFXID_WIZARD_WATTACK, SND_MAXVOL, ATTN_NORM);
  edict_t *spike = utl_launch_spike(self->v.owner, &self->v.origin, &pr.v_angles, &dir, 600);
  spike->v.effects |= EF_TRACER;

  utl_remove(self);
}

static void wizard_fire_missile(edict_t *self) {
  utl_sound(self, CHAN_WEAPON, SFXID_WIZARD_WATTACK, SND_MAXVOL, ATTN_NORM);

  utl_makevectors(&self->v.angles);

  edict_t *delay = ED_Alloc();
  delay->v.owner = self;
  delay->v.nextthink = gs.time + FTOX(0.8);
  delay->v.extra_ptr = self->v.monster->enemy;
  delay->v.think = wizard_missile_start;
  delay->v.avelocity = pr.v_right;
  delay->v.angles = self->v.angles;
  delay->v.origin.x = self->v.origin.x + 14 * pr.v_forward.x + 14 * pr.v_right.x;
  delay->v.origin.y = self->v.origin.y + 14 * pr.v_forward.y + 14 * pr.v_right.y;
  delay->v.origin.z = self->v.origin.z + 14 * pr.v_forward.z + 14 * pr.v_right.z + TO_FIX32(30);
  G_SetSize(delay, &x32vec3_origin, &x32vec3_origin);
  G_LinkEdict(delay, false);

  delay = ED_Alloc();
  delay->v.owner = self;
  delay->v.nextthink = gs.time + FTOX(0.3);
  delay->v.extra_ptr = self->v.monster->enemy;
  delay->v.think = wizard_missile_start;
  delay->v.avelocity.x = -pr.v_right.x;
  delay->v.avelocity.y = -pr.v_right.y;
  delay->v.avelocity.z = -pr.v_right.z;
  delay->v.angles = self->v.angles;
  delay->v.origin.x = self->v.origin.x + 14 * pr.v_forward.x + 14 * pr.v_right.x;
  delay->v.origin.y = self->v.origin.y + 14 * pr.v_forward.y + 14 * pr.v_right.y;
  delay->v.origin.z = self->v.origin.z + 14 * pr.v_forward.z - 14 * pr.v_right.z + TO_FIX32(30);
  G_SetSize(delay, &x32vec3_origin, &x32vec3_origin);
  G_LinkEdict(delay, false);
}

static void wizard_attack_finished(edict_t *self) {
  if (pr.enemy_range >= RANGE_MID || !pr.enemy_vis) {
    self->v.monster->attack_state = AS_STRAIGHT;
    monster_set_next_state(self, MSTATE_RUN);
  } else {
    self->v.monster->attack_state = AS_SLIDING;
    monster_set_next_state(self, MSTATE_SIDE);
  }
}

static void wizard_missile(edict_t *self) {
  monster_finite_state(self, MSTATE_MISSILE, MSTATE_RUN);

  ai_face(self);

  if (self->v.frame == MAGATT1) {
    wizard_fire_missile(self);
  } else if (self->v.frame == MAGATT10) {
    ai_attack_finished(self, TO_FIX32(2));
    wizard_attack_finished(self);
  }
}

static void wizard_pain(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN, MSTATE_RUN);
}

static void wizard_die(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE, -1);

  if (self->v.frame == DEATH1) {
    utl_sound(self, CHAN_WEAPON, SFXID_WIZARD_WDEATH, SND_MAXVOL, ATTN_NORM);
    self->v.velocity.x = 400 * xrand32() - TO_FIX32(200);
    self->v.velocity.y = 400 * xrand32() - TO_FIX32(200);
    self->v.velocity.z = 100 * xrand32() + TO_FIX32(100);
    self->v.flags &= ~FL_ONGROUND;
  } else if (self->v.frame == DEATH3) {
    self->v.solid = SOLID_NOT;
  } else if (self->v.frame == DEATH8) {
    ai_fade_corpse(self);
  }
}

static void wizard_start_die(edict_t *self, edict_t *killer) {
  if (self->v.health < -40) {
    ai_gib(self);
    return;
  }
  monster_exec_state(self, MSTATE_DIE);
}

static void wizard_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  utl_sound(self, CHAN_WEAPON, SFXID_WIZARD_WPAIN, SND_MAXVOL, ATTN_NORM);

  if ((rand() & 63) > damage)
    return; // didn't flinch

  monster_exec_state(self, MSTATE_PAIN);
}

static qboolean wizard_check_attack(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  if (gs.time < mon->attack_finished)
    return false;

  if (!pr.enemy_vis)
    return false;

  if (pr.enemy_range == RANGE_FAR) {
    if (mon->attack_state != AS_STRAIGHT) {
      mon->attack_state = AS_STRAIGHT;
      monster_exec_state(self, MSTATE_RUN);
    }
    return false;
  }

  edict_t *targ = mon->enemy;

  // see if any entities are in the way of the shot
  x32vec3_t spot1 = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + self->v.viewheight }};
  x32vec3_t spot2 = {{ targ->v.origin.x, targ->v.origin.y, targ->v.origin.z + targ->v.viewheight }};
  const trace_t *trace = utl_traceline(&spot1, &spot2, false, self);
  if (trace->ent != targ) {
    // don't have a clear shot, so move to a side
    if (mon->attack_state != AS_STRAIGHT) {
      mon->attack_state = AS_STRAIGHT;
      monster_exec_state(self, MSTATE_RUN);
    }
    return false;
  }

  x32 chance;
  switch (pr.enemy_range) {
  case RANGE_MELEE:
    chance = FTOX(0.9);
    mon->attack_finished = 0;
    break;
  case RANGE_NEAR:
    chance = FTOX(0.6);
    break;
  case RANGE_MID:
    chance = FTOX(0.2);
    break;
  default:
    chance = 0;
    break;
  }

  if (xrand32() < chance) {
    monster_exec_state(self, MSTATE_MISSILE);
    return true;
  }

  if (pr.enemy_range == RANGE_MID) {
    if (mon->attack_state != AS_STRAIGHT) {
      mon->attack_state = AS_STRAIGHT;
      monster_exec_state(self, MSTATE_RUN);
    }
  } else {
    if (mon->attack_state != AS_SLIDING) {
      mon->attack_state = AS_SLIDING;
      monster_exec_state(self, MSTATE_SIDE);
    }
  }

  return false;
}

void spawn_monster_wizard(edict_t *self) {
  G_SetModel(self, MDLID_WIZARD);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.health = self->v.max_health = 80;
  flymonster_start(self, &monster_wizard_class);
}
