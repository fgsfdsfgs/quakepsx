#include "prcommon.h"

static inline x32 trace_random(void) {
  return 2 * (xrand32() - HALF);
}

void multidamage_clear(void) {
  pr.multi_damage = 0;
  pr.multi_ent = gs.edicts;
}

void multidamage_apply(edict_t *self) {
  if (!pr.multi_ent || pr.multi_ent == gs.edicts)
    return;
  utl_damage(pr.multi_ent, self, self, pr.multi_damage);
}

void multidamage_add(edict_t *self, edict_t *hit, const s16 damage) {
  if (!hit || hit == gs.edicts)
    return;

  if (hit != pr.multi_ent) {
    multidamage_apply(self);
    pr.multi_damage = damage;
    pr.multi_ent = hit;
  } else {
    pr.multi_damage += damage;
  }
}

void utl_killed(edict_t *self, edict_t *attacker) {
  if (self->v.health < -99)
    self->v.health = -99; // don't let sbar look bad if a player

  if (self->v.movetype == MOVETYPE_PUSH || self->v.movetype == MOVETYPE_NONE) {
    // doors, triggers, etc.
    if (self->v.th_die)
      self->v.th_die(self);
    return;
  }

  monster_fields_t *mon = (self->v.flags & FL_MONSTER) ? self->v.monster : NULL;
  if (mon) {
    mon->enemy = attacker;
    // bump the monster counter
    pr.killed_monsters++;
  }

  // TODO: ClientObituary

  self->v.flags &= ~(FL_TAKEDAMAGE | FL_AUTOAIM);
  self->v.touch = null_touch;

  monster_death_use(self);
  if (self->v.th_die)
    self->v.th_die(self);
}

void utl_damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, s16 damage) {
  if ((targ->v.flags & FL_TAKEDAMAGE) == 0)
    return;

  // used by buttons and triggers to set activator for target firing
  // TODO: damage_attacker?

  // check for quad damage powerup on the attacker
  if (attacker->v.classname == ENT_PLAYER && (attacker->v.player->stats.items & IT_QUAD))
    damage *= 4;

  // save damage based on the target's armor level
  s16 take = damage;
  player_state_t *targ_plr = targ->v.classname == ENT_PLAYER ? targ->v.player : NULL;
  if (targ_plr && targ_plr->stats.armortype) {
    s16 save = XMUL16(targ_plr->stats.armortype, damage);
    if (save >= targ_plr->stats.armor) {
      save = targ_plr->stats.armor;
      targ_plr->stats.armortype = 0; // lost all armor
      targ_plr->stats.items &= ~(IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3);
    }
    targ_plr->stats.armor -= save;
    take -= save;
  }

  if (!take)
    return;

  // figure momentum add
  if (inflictor != gs.edicts && targ->v.movetype == MOVETYPE_WALK) {
    x32 sqrlen;
    x16vec3_t dir;
    const x32vec3_t delta = {{
      targ->v.origin.x - ((inflictor->v.absmin.x + inflictor->v.absmax.x) >> 1),
      targ->v.origin.y - ((inflictor->v.absmin.y + inflictor->v.absmax.y) >> 1),
      targ->v.origin.z - ((inflictor->v.absmin.z + inflictor->v.absmax.z) >> 1)
    }};
    XVecNormLS(&delta, &dir, &sqrlen);
    targ->v.velocity.x += (x32)dir.x * (x32)damage * 8;
    targ->v.velocity.y += (x32)dir.y * (x32)damage * 8;
    targ->v.velocity.z += (x32)dir.z * (x32)damage * 8;
  }

  // check for godmode or invincibility
  if (targ->v.flags & FL_GODMODE)
    return;

  if (targ_plr) {
    if (targ_plr->stats.items & IT_INVULNERABILITY) {
      // TODO: invincible_sound
      return;
    }
    // pain sound and effects
    player_pain(targ, attacker, damage);
  }

  // do the damage
  targ->v.health -= take;
  if (targ->v.health <= 0) {
    utl_killed(targ, attacker);
    return;
  }

  // react to the damage
  if (targ->v.flags & FL_MONSTER) {
    // get mad unless of the same class (except for soldiers)
    if (attacker != gs.edicts && targ != attacker && attacker != targ->v.monster->enemy) {
      if ((targ->v.classname != attacker->v.classname) || targ->v.classname == ENT_MONSTER_ARMY) {
        if (targ->v.monster->enemy && targ->v.monster->enemy->v.classname == ENT_PLAYER)
          targ->v.monster->oldenemy = targ->v.monster->enemy;
        targ->v.monster->enemy = attacker;
        ai_foundtarget(targ);
      }
    }
    // play pain animation if present
    if (targ->v.monster->class->fn_start_pain) {
      targ->v.monster->class->fn_start_pain(targ, attacker, take);
      // nightmare mode monsters don't go into pain frames often
      if (/* skill == 3*/ 0)
        targ->v.monster->pain_finished = gs.time + TO_FIX32(5);
    }
  }
}

void utl_traceattack(edict_t *self, s16 damage, const x32vec3_t *dir) {
  x32vec3_t org = pr.trace->endpos;
  org.x -= dir->x * 4;
  org.y -= dir->y * 4;
  org.z -= dir->z * 4;
  if (pr.trace->ent->v.flags & FL_TAKEDAMAGE)
    multidamage_add(self, pr.trace->ent, damage);
  // TODO: particles
}

void utl_firebullets(edict_t *self, int shotcount, const x16vec3_t *dir, const x16 spread_x, const x16 spread_y) {
  utl_makevectors(&self->v.angles);

  x32vec3_t src = {{
    self->v.origin.x + pr.v_forward.x * 10,
    self->v.origin.y + pr.v_forward.y * 10,
    self->v.absmin.z + xmul32(FTOX(0.7), self->v.size.z)
  }};

  multidamage_clear();

  // TODO: particles
  x32vec3_t direction;
  x32vec3_t end;
  for (; shotcount > 0; --shotcount) {
    const x32 r_up = XMUL16(trace_random(), spread_y);
    const x32 r_rt = XMUL16(trace_random(), spread_x);
    direction.x = dir->x + XMUL16(r_up, pr.v_up.x) + XMUL16(r_rt, pr.v_right.x);
    direction.y = dir->y + XMUL16(r_up, pr.v_up.y) + XMUL16(r_rt, pr.v_right.y);
    direction.z = dir->z + XMUL16(r_up, pr.v_up.z) + XMUL16(r_rt, pr.v_right.z);
    end.x = src.x + direction.x * 2048;
    end.y = src.y + direction.y * 2048;
    end.z = src.z + direction.z * 2048;
    utl_traceline(&src, &end, false, self);
    if (pr.trace->fraction != ONE)
      utl_traceattack(self, 4, &direction);
  }

  multidamage_apply(self);
  // TODO: particles
}
