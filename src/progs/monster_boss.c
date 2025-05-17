#include "prcommon.h"

  // HACK: the og code tries to look for targetname "lightning",
  // but we don't have a way to reconstruct targetnames here, so...
  // maybe replace the current system with hashes or something?
#define PILLAR_TARGET 0x0e

enum boss_frames_e {
  RISE1, RISE2, RISE3, RISE4, RISE5, RISE6, RISE7, RISE8, RISE9, RISE10,
  RISE11, RISE12, RISE13, RISE14, RISE15, RISE16, RISE17,

  WALK1, WALK2, WALK3, WALK4, WALK5, WALK6, WALK7, WALK8,
  WALK9, WALK10, WALK11, WALK12, WALK13, WALK14, WALK15,
  WALK16, WALK17, WALK18, WALK19, WALK20, WALK21, WALK22,
  WALK23, WALK24, WALK25, WALK26, WALK27, WALK28, WALK29, WALK30, WALK31,

  DEATH1, DEATH2, DEATH3, DEATH4, DEATH5, DEATH6, DEATH7, DEATH8, DEATH9,

  ATTACK1, ATTACK2, ATTACK3, ATTACK4, ATTACK5, ATTACK6, ATTACK7, ATTACK8,
  ATTACK9, ATTACK10, ATTACK11, ATTACK12, ATTACK13, ATTACK14, ATTACK15,
  ATTACK16, ATTACK17, ATTACK18, ATTACK19, ATTACK20, ATTACK21, ATTACK22,
  ATTACK23,

  SHOCKA1, SHOCKA2, SHOCKA3, SHOCKA4, SHOCKA5, SHOCKA6, SHOCKA7, SHOCKA8,
  SHOCKA9, SHOCKA10,

  SHOCKB1, SHOCKB2, SHOCKB3, SHOCKB4, SHOCKB5, SHOCKB6,

  SHOCKC1, SHOCKC2, SHOCKC3, SHOCKC4, SHOCKC5, SHOCKC6, SHOCKC7, SHOCKC8,
  SHOCKC9, SHOCKC10
};

static void boss_idle(edict_t *self);
static void boss_missile(edict_t *self);
static void boss_rise(edict_t *self);
static void boss_shock_a(edict_t *self);
static void boss_shock_b(edict_t *self);
static void boss_shock_c(edict_t *self);
static void boss_die(edict_t *self);

static const monster_state_t monster_boss_states[MSTATE_COUNT] = {
  /* STAND   */ { boss_idle,    WALK1,   WALK31   },
  /* WALK    */ { NULL,         -1,      -1       },
  /* RUN     */ { NULL,         -1,      -1       },
  /* MISSILE */ { boss_missile, ATTACK1, ATTACK23 },
  /* MELEE   */ { NULL,         -1,      -1       },
  /* DIE_A   */ { boss_die,     DEATH1,  DEATH9   },
  /* DIE_B   */ { NULL,         -1,       -1      },
  /* PAIN_A  */ { boss_shock_a, SHOCKA1, SHOCKA10 },
  /* PAIN_B  */ { boss_shock_b, SHOCKB1, SHOCKB6  },
  /* PAIN_C  */ { boss_shock_c, SHOCKC1, SHOCKC10 },
  /* PAIN_D  */ { NULL,         -1,      -1       },
  /* PAIN_E  */ { NULL,         -1,      -1       },
  /* EXTRA   */ { boss_rise,    RISE1,   RISE17   },
};

static const monster_class_t monster_boss_class = {
  .state_table = monster_boss_states,
  .fn_start_pain = NULL,
  .fn_start_die = NULL,
  .fn_check_attack = NULL,
  .sight_sound = SFXID_SOLDIER_SIGHT1
};

static void boss_face(edict_t *self) {
  // go for another player if multi player
  if (self->v.monster->enemy->v.health <= 0 || xrand32() < FTOX(0.02)) {
    // TODO
  }
  ai_face(self);
}

static void boss_fire_missile(edict_t *self, const x32vec3_t *p) {
  utl_sound(self, CHAN_WEAPON, SFXID_BOSS1_THROW, SND_MAXVOL, ATTN_NORM);

  edict_t *enemy = self->v.monster->enemy;
  x32vec3_t delta;
  x16vec3_t dir;
  x32 xysqrlen;
  XVecSub(&enemy->v.origin, &self->v.origin, &delta);
  XVecNormLS(&delta, &dir, &xysqrlen);
  utl_vectoangles(&dir);
  utl_makevectors(&pr.v_angles);

  x32vec3_t org;
  org.x = self->v.origin.x + xmul32(p->x, pr.v_forward.x) + xmul32(p->y, pr.v_right.x);
  org.y = self->v.origin.y + xmul32(p->x, pr.v_forward.y) + xmul32(p->y, pr.v_right.y);
  org.z = self->v.origin.z + xmul32(p->x, pr.v_forward.z) + xmul32(p->y, pr.v_right.z) + p->z;

  // lead the player on hard mode
  x32vec3_t d;
  if (gs.skill > 1) {
    XVecSub(&enemy->v.origin, &org, &delta);
    const x32 len = xdiv32(TO_FIX32(XVecLengthIntL(&delta)), TO_FIX32(300));
    const x32vec3_t vec = {{
      xmul32(len, enemy->v.velocity.x),
      xmul32(len, enemy->v.velocity.y),
      0
    }};
    XVecAdd(&enemy->v.origin, &vec, &d);
  } else {
    d = enemy->v.origin;
  }

  XVecSub(&d, &org, &d);
  XVecNormLS(&d, &dir, &xysqrlen);

  edict_t *newmis = utl_launch_rocket(self, &org, &x16vec3_origin, &dir, 300);
  G_SetModel(newmis, MDLID_LAVABALL);
  newmis->v.avelocity.x = TO_DEG16(200);
  newmis->v.avelocity.y = TO_DEG16(100);
  newmis->v.avelocity.z = TO_DEG16(300);

  // check for dead enemy
  if (enemy->v.health <= 0)
    monster_set_state(self, MSTATE_STAND);
}

static void boss_rise(edict_t *self) {
  if (self->v.frame == RISE1)
    utl_sound(self, CHAN_WEAPON, SFXID_BOSS1_OUT1, SND_MAXVOL, ATTN_NORM);
  else if (self->v.frame == RISE2)
    utl_sound(self, CHAN_VOICE, SFXID_BOSS1_SIGHT1, SND_MAXVOL, ATTN_NORM);
  monster_end_state(self, MSTATE_EXTRA, MSTATE_MISSILE);
}

static void boss_idle(edict_t *self) {
  if (self->v.frame > WALK1)
    boss_face(self);
  monster_loop_state(self, MSTATE_STAND);
}

static void boss_missile(edict_t *self) {
  x32vec3_t p;
  switch (self->v.frame) {
  case ATTACK9:
    XVecSetInt(&p, 100, 100, 200);
    boss_fire_missile(self, &p);
    break;
  case ATTACK20:
    XVecSetInt(&p, 100, -100, 200);
    boss_fire_missile(self, &p);
    break;
  default:
    boss_face(self);
    break;
  }
  monster_loop_state(self, MSTATE_MISSILE);
}

static void boss_shock_a(edict_t *self) {
  monster_end_state(self, MSTATE_PAIN_A, MSTATE_MISSILE);
}

static void boss_shock_b(edict_t *self) {
  monster_end_state(self, MSTATE_PAIN_B, MSTATE_MISSILE);
}

static void boss_shock_c(edict_t *self) {
  monster_end_state(self, MSTATE_PAIN_C, MSTATE_DIE_A);
}

static void boss_die(edict_t *self) {
  switch (self->v.frame) {
  case DEATH1:
    utl_sound(self, CHAN_VOICE, SFXID_BOSS1_DEATH, SND_MAXVOL, ATTN_NORM);
    break;
  case DEATH8:
    utl_sound(self, CHAN_WEAPON, SFXID_BOSS1_OUT1, SND_MAXVOL, ATTN_NORM);
    R_SpawnParticleLavaSplash(&self->v.origin);
    break;
  case DEATH9:
    pr.killed_monsters++;
    utl_usetargets(self, self->v.monster->enemy);
    utl_remove(self);
    return;
  default:
    break;
  }
  monster_end_state(self, MSTATE_DIE_A, -1);
}

static void boss_awake(edict_t *self, edict_t *activator) {
  self->v.flags |= FL_MONSTER;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  G_SetModel(self, MDLID_BOSS);

  if (gs.skill == 0)
    self->v.health = self->v.max_health = 1;
  else
    self->v.health = self->v.max_health = 3;

  self->v.monster->enemy = activator;
  self->v.monster->yaw_speed = TO_DEG16(20);

  G_LinkEdict(self, false);

  R_SpawnParticleLavaSplash(&self->v.origin);

  monster_set_state(self, MSTATE_EXTRA);
}

void spawn_monster_boss(edict_t *self) {
  monster_fields_t *mon = Mem_ZeroAlloc(sizeof(monster_fields_t));
  mon->class = &monster_boss_class;
  mon->enemy = gs.edicts;
  mon->oldenemy = gs.edicts;
  mon->movetarget = gs.edicts;
  mon->goalentity = gs.edicts;
  mon->state_num = MSTATE_STAND;
  mon->state = &monster_boss_states[0];

  self->v.monster = mon;
  self->v.use = boss_awake;
  self->v.model = NULL;
  self->v.modelnum = 0;
  XVecSetInt(&self->v.mins, -128, -128, -24);
  XVecSetInt(&self->v.maxs, +128, +128, +256);
  G_SetSize(self, &self->v.mins, &self->v.maxs);

  pr.total_monsters++;
}

static void lightning_fire(edict_t *self) {
  if (gs.time >= self->v.extra_trigger.wait) {
    // done here, put the terminals back up
    edict_t *le1 = G_FindByTarget(gs.edicts, PILLAR_TARGET);
    edict_t *le2 = G_FindByTarget(le1, PILLAR_TARGET);
    if (le1 && le2) {
      door_go_down(le1);
      door_go_down(le2);
    }
    return;
  }

  // TODO: lightning

  self->v.nextthink = gs.time + PR_FRAMETIME;
  self->v.think = lightning_fire;
}

static void lightning_use(edict_t *self, edict_t *activator) {
  if (self->v.extra_trigger.wait >= gs.time + ONE)
    return;

  edict_t *le1 = G_FindByTarget(gs.edicts, PILLAR_TARGET);
  edict_t *le2 = G_FindByTarget(le1, PILLAR_TARGET);
  if (!le1 || !le2 || !le1->v.door || !le2->v.door) {
    Sys_Printf("missing lightning targets\n");
    return;
  }

  if ((le1->v.door->state != STATE_TOP && le1->v.door->state != STATE_BOTTOM)
    || (le2->v.door->state != STATE_TOP && le2->v.door->state != STATE_BOTTOM)
    || (le1->v.door->state != le2->v.door->state))
    return;

  // don't let the electrodes go back up until the bolt is done
  le1->v.nextthink = -1;
  le2->v.nextthink = -1;
  self->v.extra_trigger.wait = gs.time + ONE;

  utl_sound(self, CHAN_VOICE, SFXID_MISC_POWER, SND_MAXVOL, ATTN_NORM);
  lightning_fire(self);

  // advance the boss pain if down
  self = G_FindByClassname(gs.edicts, ENT_MONSTER_BOSS);
  if (!self || !self->v.monster)
    return;

  self->v.monster->enemy = activator;

  if (le1->v.door->state == STATE_TOP && self->v.health > 0) {
    utl_sound(self, CHAN_VOICE, SFXID_BOSS1_PAIN, SND_MAXVOL, ATTN_NORM);
    self->v.health--;
    if (self->v.health >= 2)
      monster_set_state(self, MSTATE_PAIN_A);
    else if (self->v.health == 1)
      monster_set_state(self, MSTATE_PAIN_B);
    else if (self->v.health == 0)
      monster_set_state(self, MSTATE_PAIN_C);
  }
}

void spawn_event_lightning(edict_t *self) {
  self->v.use = lightning_use;
}
