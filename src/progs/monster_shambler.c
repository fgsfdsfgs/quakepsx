#include "prcommon.h"

enum shambler_frames_e {
  STAND1, STAND2, STAND3, STAND4, STAND5, STAND6, STAND7, STAND8, STAND9,
  STAND10, STAND11, STAND12, STAND13, STAND14, STAND15, STAND16, STAND17,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7,
  WALK8, WALK9, WALK10, WALK11, WALK12,

  RUN1, RUN2, RUN3, RUN4, RUN5, RUN6,

  SMASH1, SMASH2, SMASH3, SMASH4, SMASH5, SMASH6, SMASH7,
  SMASH8, SMASH9, SMASH10, SMASH11, SMASH12,

  SWINGR1, SWINGR2, SWINGR3, SWINGR4, SWINGR5,
  SWINGR6, SWINGR7, SWINGR8, SWINGR9,

  SWINGL1, SWINGL2, SWINGL3, SWINGL4, SWINGL5,
  SWINGL6, SWINGL7, SWINGL8, SWINGL9,

  MAGIC1, MAGIC2, MAGIC3, MAGIC4, MAGIC5,
  MAGIC6, MAGIC7, MAGIC8, MAGIC9, MAGIC10, MAGIC11, MAGIC12,

  PAIN1, PAIN2, PAIN3, PAIN4, PAIN5, PAIN6,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6,
  DEATH7, DEATH8, DEATH9, DEATH10, DEATH11,
};

enum shambler_states_e {
  MSTATE_SWINGL = MSTATE_EXTRA,
  MSTATE_SWINGR,
  MSTATE_DIE,
  MSTATE_PAIN,
};

static void sham_stand(edict_t *self);
static void sham_walk(edict_t *self);
static void sham_run(edict_t *self);
static void sham_magic(edict_t *self);
static void sham_smash(edict_t *self);
static void sham_swingl(edict_t *self);
static void sham_swingr(edict_t *self);
static void sham_pain(edict_t *self);
static void sham_die(edict_t *self);

static void sham_start_die(edict_t *self, edict_t *killer);
static void sham_start_pain(edict_t *self, edict_t *attacker, s16 damage);
static qboolean sham_check_attack(edict_t *self);

static const monster_state_t monster_shambler_states[MSTATE_MAX] = {
  /* STAND   */ { sham_stand,  STAND1,  STAND17 },
  /* WALK    */ { sham_walk,   WALK1,   WALK12  },
  /* RUN     */ { sham_run,    RUN1,    RUN6    },
  /* MISSILE */ { sham_magic,  MAGIC1,  MAGIC12 },
  /* MELEE   */ { sham_smash,  SMASH1,  SMASH12 },
  /* SWINGL  */ { sham_swingl, SWINGL1, SWINGL9 },
  /* SWINGR  */ { sham_swingr, SWINGR1, SWINGR9 },
  /* DIE     */ { sham_die,    DEATH1,  DEATH11 },
  /* PAIN    */ { sham_pain,   PAIN1,   PAIN6   },
};

static const monster_class_t monster_shambler_class = {
  .state_table = monster_shambler_states,
  .fn_start_pain = sham_start_pain,
  .fn_start_die = sham_start_die,
  .fn_check_attack = sham_check_attack,
  .sight_sound = SFXID_SHAMBLER_SSIGHT
};

static void sham_stand(edict_t *self) {
  monster_looping_state(self, MSTATE_STAND);
  ai_stand(self);
}

static void sham_walk(edict_t *self) {
  static const s8 walktab[] = {
    10, 9, 9, 5, 6, 12, 8, 3, 13, 9, 7, 7
  };

  monster_looping_state(self, MSTATE_WALK);

  ai_walk(self, TO_FIX32(walktab[self->v.frame - WALK1]));

  if (self->v.frame == WALK12)
    if (xrand32() > FTOX(0.8))
      utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SIDLE, SND_MAXVOL, ATTN_IDLE);
}

static void sham_run(edict_t *self) {
  static const s8 runtab[] = {
    20, 24, 20, 20, 24, 20
  };

  monster_looping_state(self, MSTATE_RUN);

  ai_run(self, TO_FIX32(runtab[self->v.frame - RUN1]));

  if (self->v.frame == RUN6)
    if (xrand32() > FTOX(0.8))
      utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SIDLE, SND_MAXVOL, ATTN_IDLE);
}

static void sham_cast_lightning(edict_t *self) {
  ai_face(self);

  const edict_t *enemy = self->v.monster->enemy;

  const x32vec3_t org = {{
    self->v.origin.x,
    self->v.origin.y,
    self->v.origin.z + TO_FIX32(40)
  }};

  x32vec3_t end = {{
    enemy->v.origin.x - org.x,
    enemy->v.origin.y - org.y,
    enemy->v.origin.z + TO_FIX32(16) - org.z
  }};
  x16vec3_t dir;
  x32 xysqrlen;
  XVecNormLS(&end, &dir, &xysqrlen);

  end.x = self->v.origin.x + 600 * dir.x;
  end.y = self->v.origin.y + 600 * dir.y;
  end.z = self->v.origin.z + 600 * dir.z;

  R_SpawnBeam(&org, &end, 0xFFC0C0, 2, -1);
  utl_traceline(&org, &end, true, self);
  utl_lightning_damage(self, &org, &pr.trace->endpos, self, 10);
}

static void sham_magic(edict_t *self) {
  monster_finite_state(self, MSTATE_MISSILE, MSTATE_RUN);

  switch (self->v.frame) {
  case MAGIC1:
    ai_face(self);
    utl_sound(self, CHAN_WEAPON, SFXID_SHAMBLER_SATTCK1, SND_MAXVOL, ATTN_NORM);
    break;
  case MAGIC2:
    ai_face(self);
    break;
  case MAGIC3:
    ai_face(self);
    self->v.nextthink += PR_FRAMETIME * 2;
    self->v.effects |= EF_MUZZLEFLASH;
    // TODO: little lightning between hands
    break;
  case MAGIC4:
  case MAGIC5:
    self->v.effects |= EF_MUZZLEFLASH;
    // TODO: little lightning between hands
    break;
  case MAGIC6:
    self->v.effects |= EF_MUZZLEFLASH;
    sham_cast_lightning(self);
    break;
  case MAGIC9:
  case MAGIC10:
    sham_cast_lightning(self);
    break;
  case MAGIC11:
    if (gs.skill == 3)
      sham_cast_lightning(self);
    break;
  default:
    break;
  }
}

static void sham_attack_smash(edict_t *self) {
  edict_t *enemy = self->v.monster->enemy;
  if (!enemy || enemy == gs.edicts)
    return;

  ai_charge(self, 0);

  x32vec3_t delta;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  if (XVecLengthSqrIntL(&delta) > 100 * 100)
    return;

  if (!utl_can_damage(self, enemy, self))
    return;

  const s16 ldmg = xmul32(xrand32() + xrand32() + xrand32(),  4);
  utl_damage(enemy, self, self, ldmg);
  utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SMACK, SND_MAXVOL, ATTN_NORM);
  fx_spawn_blood(&enemy->v.origin, ldmg); // TODO
}

static void sham_attack_claw(edict_t *self, const s16 side) {
  edict_t *enemy = self->v.monster->enemy;
  if (!enemy || enemy == gs.edicts)
    return;

  ai_charge(self, 10);

  x32vec3_t delta;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  if (XVecLengthSqrIntL(&delta) > 100 * 100)
    return;

  const s16 ldmg = xmul32(xrand32() + xrand32() + xrand32(),  20);
  utl_damage(enemy, self, self, ldmg);
  utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SMACK, SND_MAXVOL, ATTN_NORM);

  if (side)
    fx_spawn_blood(&enemy->v.origin, ldmg); // TODO
}

static void sham_smash(edict_t *self) {
  static const s8 disttab[] = {
    2, 6, 6, 5, 4, 1, 0, 0, 0, 0, 5, 4
  };

  if (self->v.monster->next_frame == SMASH1) {
    // randomly switch to claw attacks if not at full health
    const x32 r = xrand32();
    if (self->v.health != self->v.max_health && r <= FTOX(0.6)) {
      if (r > FTOX(0.3))
        monster_exec_state(self, MSTATE_SWINGR);
      else
        monster_exec_state(self, MSTATE_SWINGL);
      return;
    }
  }

  monster_finite_state(self, MSTATE_MELEE, MSTATE_RUN);

  switch (self->v.frame) {
  case SMASH1:
    utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_MELEE1, SND_MAXVOL, ATTN_NORM);
    /* fallthrough */
  default:
    ai_charge(self, TO_FIX32(disttab[self->v.frame - SMASH1]));
    break;
  case SMASH10:
    sham_attack_smash(self);
    break;
  }
}

static void sham_swingl(edict_t *self) {
  static const s8 disttab[] = {
    5, 3, 7, 3, 7, 9, 5, 4, 8
  };

  monster_finite_state(self, MSTATE_SWINGL, MSTATE_RUN);

  ai_charge(self, TO_FIX32(disttab[self->v.frame - SWINGL1]));

  switch (self->v.frame) {
  case SWINGL1:
    utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_MELEE1, SND_MAXVOL, ATTN_NORM);
    break;
  case SWINGL7:
    sham_attack_claw(self, 250);
    break;
  case SWINGL9:
    if (xrand32() < HALF)
      monster_set_next_state(self, MSTATE_SWINGR);
    break;
  default:
    break;
  }
}

static void sham_swingr(edict_t *self) {
  static const s8 disttab[] = {
    1, 8, 14, 7, 3, 6, 6, 3, 11
  };

  monster_finite_state(self, MSTATE_SWINGR, MSTATE_RUN);

  ai_charge(self, TO_FIX32(disttab[self->v.frame - SWINGR1]));

  switch (self->v.frame) {
  case SWINGR1:
    utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_MELEE2, SND_MAXVOL, ATTN_NORM);
    break;
  case SWINGR7:
    sham_attack_claw(self, -250);
    break;
  case SWINGR9:
    if (xrand32() < HALF)
      monster_set_next_state(self, MSTATE_SWINGL);
    break;
  default:
    break;
  }
}

static void sham_pain(edict_t *self) {
  monster_finite_state(self, MSTATE_PAIN, MSTATE_RUN);
}

static void sham_die(edict_t *self) {
  monster_finite_state(self, MSTATE_DIE, -1);
  if (self->v.frame == DEATH3)
    self->v.solid = SOLID_NOT;
}

static void sham_start_die(edict_t *self, edict_t *killer) {
  if (self->v.health < -60) {
    ai_gib(self);
    return;
  }

  utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SDEATH, SND_MAXVOL, ATTN_NORM);
  monster_exec_state(self, MSTATE_DIE);
}

static void sham_start_pain(edict_t *self, edict_t *attacker, s16 damage) {
  utl_sound(self, CHAN_VOICE, SFXID_SHAMBLER_SHURT2, SND_MAXVOL, ATTN_NORM);

  if (self->v.health <= 0)
    return; // already dying

  if (self->v.monster->pain_finished > gs.time)
    return;

  if (XMUL16(400, xrand32()) > damage)
    return; // didn't flinch

  self->v.monster->pain_finished = gs.time + TO_FIX32(2);
  monster_exec_state(self, MSTATE_PAIN);
}

static qboolean sham_check_attack(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  if (pr.enemy_range == RANGE_MELEE) {
    if (utl_can_damage(self, mon->enemy, self)) {
      mon->attack_state = AS_MELEE;
      return false;
    }
  }

  if (gs.time < mon->attack_finished)
    return false;

  if (!pr.enemy_vis)
    return false;

  // missile attack
  if (pr.enemy_range == RANGE_FAR)
    return false;

  edict_t *targ = mon->enemy;
  const x32vec3_t spot1 = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + self->v.viewheight }};
  const x32vec3_t spot2 = {{ targ->v.origin.x, targ->v.origin.y, targ->v.origin.z + targ->v.viewheight }};
  x32vec3_t delta;
  XVecSub(&spot2, &spot1, &delta);
  if (XVecLengthSqrIntL(&delta) > 600 * 600)
    return false;

  // see if any entities are in the way of the shot
  const trace_t *trace = utl_traceline(&spot1, &spot2, false, self);
  if (trace->ent != targ)
    return false; // no clear shot
  if (trace->inopen && trace->inwater)
    return false; // sight line crossed contents

  mon->attack_state = AS_MISSILE;
  ai_attack_finished(self, TO_FIX32(2) + 2 * xrand32());
  return true;
}

void spawn_monster_shambler(edict_t *self) {
  G_SetModel(self, MDLID_SHAMBLER);
  G_SetSize(self, &gs.worldmodel->hulls[2].mins, &gs.worldmodel->hulls[2].maxs);
  self->v.health = self->v.max_health = 600;
  walkmonster_start(self, &monster_shambler_class);
}
