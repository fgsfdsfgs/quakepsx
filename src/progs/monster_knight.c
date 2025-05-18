#include "prcommon.h"

enum knight_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8, STAND9,

  RUNB1, RUNB2, RUNB3, RUNB4, RUNB5, RUNB6, RUNB7, RUNB8,

  RUNATTACK1, RUNATTACK2, RUNATTACK3, RUNATTACK4, RUNATTACK5,
  RUNATTACK6, RUNATTACK7, RUNATTACK8, RUNATTACK9, RUNATTACK10,
  RUNATTACK11,

  PAIN1, PAIN2, PAIN3,

  PAINB1, PAINB2, PAINB3, PAINB4, PAINB5, PAINB6, PAINB7, PAINB8, PAINB9,
  PAINB10, PAINB11,

  // BUG?: first frame here was labeled ATTACKB1 as well, not sure how Quake handles this
  ATTACKB0, ATTACKB1, ATTACKB2, ATTACKB3, ATTACKB4, ATTACKB5,
  ATTACKB6, ATTACKB7, ATTACKB8, ATTACKB9, ATTACKB10,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7, WALK8, WALK9,
  WALK10, WALK11, WALK12, WALK13, WALK14,

  KNEEL1, KNEEL2, KNEEL3, KNEEL4, KNEEL5,

  STANDING2, STANDING3, STANDING4, STANDING5,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8,
  DEATH9, DEATH10,

  DEATHB1, DEATHB2, DEATHB3, DEATHB4, DEATHB5, DEATHB6, DEATHB7, DEATHB8,
  DEATHB9, DEATHB10, DEATHB11,
};

enum knight_states_e {
  MSTATE_RUNATK = MSTATE_EXTRA,
  MSTATE_DIE_A,
  MSTATE_DIE_B,
  MSTATE_PAIN_A,
  MSTATE_PAIN_B,
};

static void knight_stand(edict_t *self);
static void knight_walk(edict_t *self);
static void knight_run(edict_t *self);
static void knight_atk(edict_t *self);
static void knight_runatk(edict_t *self);
static void knight_pain_a(edict_t *self);
static void knight_pain_b(edict_t *self);
static void knight_die_a(edict_t *self);
static void knight_die_b(edict_t *self);

static void knight_start_die(edict_t *self, edict_t *killer);
static void knight_start_pain(edict_t *self, edict_t *attacker, s16 damage);

static const monster_state_t monster_knight_states[MSTATE_MAX] = {
  /* STAND   */ { knight_stand,   STAND1,     STAND9      },
  /* WALK    */ { knight_walk,    WALK1,      WALK14      },
  /* RUN     */ { knight_run,     RUNB1,      RUNB8       },
  /* MISSILE */ { NULL,           -1,         -1          },
  /* MELEE   */ { knight_atk,     ATTACKB1,   ATTACKB10   },
  /* RUNATK  */ { knight_runatk,  RUNATTACK1, RUNATTACK11 },
  /* DIE_A   */ { knight_die_a,   DEATH1,     DEATH10     },
  /* DIE_B   */ { knight_die_b,   DEATHB1,    DEATHB11    },
  /* PAIN_A  */ { knight_pain_a,  PAIN1,      PAIN3       },
  /* PAIN_B  */ { knight_pain_b,  PAINB1,     PAINB11     },
};

static const monster_class_t monster_knight_class = {
  .state_table = monster_knight_states,
  .fn_start_pain = knight_start_pain,
  .fn_start_die = knight_start_die,
  .fn_check_attack = NULL,
  .sight_sound = SFXID_KNIGHT_KSIGHT
};

static void knight_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  ai_stand(self);
}

static void knight_walk(edict_t *self) {
  static const s8 walktab[] = {
    3, 2, 3, 4, 3, 3, 3, 4, 3, 3, 2, 3, 4, 3
  };

  monster_looping_state(self, MSTATE_WALK);

  if (self->v.frame == WALK1 && xrand32() < FTOX(0.2))
    utl_sound(self, CHAN_VOICE, SFXID_KNIGHT_IDLE, SND_MAXVOL, ATTN_IDLE);

  ai_walk(self, TO_FIX32(walktab[self->v.frame - WALK1]));
}

static void knight_run(edict_t *self) {
  static const s8 runtab[] = {
    16, 20, 13, 7, 16, 20, 14, 6
  };

  monster_looping_state(self, MSTATE_RUN);

  if (self->v.frame == RUNB1 && xrand32() < FTOX(0.2))
    utl_sound(self, CHAN_VOICE, SFXID_KNIGHT_IDLE, SND_MAXVOL, ATTN_IDLE);

  ai_run(self, TO_FIX32(runtab[self->v.frame - RUNB1]));
}

static void knight_atk(edict_t *self) {
  static const s8 disttab[] = {
    0, 7, 4, 0, 3, 4, 1, 3, 1, 5
  };

  if (self->v.monster->next_frame == ATTACKB1) {
    // decide if now is a good swing time
    const edict_t *enemy = self->v.monster->enemy;
    const x32vec3_t sorg = {{
      self->v.origin.x,
      self->v.origin.y,
      self->v.origin.z + self->v.viewheight,
    }};
    const x32vec3_t eorg = {{
      enemy->v.origin.x,
      enemy->v.origin.y,
      enemy->v.origin.z + enemy->v.viewheight,
    }};
    x32vec3_t delta;
    XVecSub(&eorg, &sorg, &delta);
    if (XVecLengthSqrIntL(&delta) >= 80 * 80) {
      // enemy is too far; chain into running attack instead
      monster_exec_state(self, MSTATE_RUNATK);
      return;
    }
  }

  monster_finite_state(self, MSTATE_MELEE, MSTATE_RUN);

  ai_charge(self, TO_FIX32(disttab[self->v.frame - ATTACKB1]));

  switch (self->v.frame) {
  case ATTACKB1: utl_sound(self, CHAN_WEAPON, SFXID_KNIGHT_SWORD1, SND_MAXVOL, ATTN_NORM); break;
  case ATTACKB6: ai_melee(self); break;
  case ATTACKB7: ai_melee(self); break;
  case ATTACKB8: ai_melee(self); break;
  default: break;
  }
}

static void knight_runatk(edict_t *self) {
  monster_finite_state(self, MSTATE_RUNATK, MSTATE_RUN);

  switch (self->v.frame) {
  case RUNATTACK1:
    utl_sound(self, CHAN_WEAPON, SFXID_KNIGHT_SWORD1, SND_MAXVOL, ATTN_NORM);
    ai_charge(self, TO_FIX32(20));
    break;
  case RUNATTACK2 ... RUNATTACK4:
  case RUNATTACK10:
    ai_charge_side(self);
    break;
  case RUNATTACK5 ... RUNATTACK9:
    ai_melee_side(self);
    break;
  case RUNATTACK11:
    ai_charge(self, TO_FIX32(10));
    break;
  default: break;
  }
}

static void knight_pain_a(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN_A, MSTATE_RUN);
}

static void knight_pain_b(edict_t *self) {
  static const s8 paintab[] = {
    0, 3, -1, -1, 2, 4, 2, 5, 5, 0, -1
  };

  monster_finite_state(self, MSTATE_PAIN_B, MSTATE_RUN);

  const s8 d = paintab[self->v.frame - PAINB1];
  if (d >= 0)
    ai_painforward(self, TO_FIX32(d));
}

static void knight_die_a(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE_A, -1);
  if (self->v.frame == DEATH3)
    self->v.solid = SOLID_NOT;
}

static void knight_die_b(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE_B, -1);
  if (self->v.frame == DEATHB3)
    self->v.solid = SOLID_NOT;
}

static void knight_start_die(edict_t *self, edict_t *killer) {
  if (self->v.health < -40) {
    ai_gib(self);
    return;
  }

  utl_sound(self, CHAN_VOICE, SFXID_KNIGHT_KDEATH, SND_MAXVOL, ATTN_NORM);

  if (xrand32() < HALF)
    monster_exec_state(self, MSTATE_DIE_A);
  else
    monster_exec_state(self, MSTATE_DIE_B);
}

static void knight_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  if (self->v.monster->pain_finished > gs.time)
    return;

  utl_sound(self, CHAN_VOICE, SFXID_KNIGHT_KHURT, SND_MAXVOL, ATTN_NORM);

  const x32 r = xrand32();
  if (r < FTOX(0.85))
    monster_exec_state(self, MSTATE_PAIN_A);
  else
    monster_exec_state(self, MSTATE_PAIN_B);

  self->v.monster->pain_finished = gs.time + ONE;
}

void spawn_monster_knight(edict_t *self) {
  G_SetModel(self, MDLID_KNIGHT);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.health = self->v.max_health = 75;
  walkmonster_start(self, &monster_knight_class);
}
