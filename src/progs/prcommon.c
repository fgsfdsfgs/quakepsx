#include <string.h>
#include "prcommon.h"

pr_globals_t pr;

void null_think(edict_t *self) {

}

void null_touch(edict_t *self, edict_t *other) {

}

const trace_t *utl_traceline(x32vec3_t *v1, x32vec3_t *v2, const qboolean nomonsters, edict_t *ent) {
  pr.trace = G_Move(v1, &x32vec3_origin, &x32vec3_origin, v2, nomonsters, ent);
  return pr.trace;
}

void utl_makevectors(const x16vec3_t *angles) {
  AngleVectors(angles, &pr.v_forward, &pr.v_right, &pr.v_up);
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
    // signal to HUD that we're getting damaged
    Sbar_IndicateDamage(take);
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

// called on map change
void Progs_NewMap(void) {
  memset(&pr, 0, sizeof(pr));
}
