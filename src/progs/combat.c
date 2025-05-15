#include "prcommon.h"

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
      self->v.th_die(self, attacker);
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
    self->v.th_die(self, attacker);
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
      if (gs.skill == 3)
        targ->v.monster->pain_finished = gs.time + TO_FIX32(5);
    }
  }
}

void utl_traceattack(edict_t *self, s16 damage, const x32vec3_t *dir) {
  x32vec3_t org = pr.trace->endpos;
  org.x -= dir->x * 4;
  org.y -= dir->y * 4;
  org.z -= dir->z * 4;
  if (pr.trace->ent->v.flags & FL_TAKEDAMAGE) {
    multidamage_add(self, pr.trace->ent, damage);
    fx_spawn_blood(&org, damage);
  } else {
    fx_spawn_gunshot(&org);
  }
}

void utl_firebullets(edict_t *self, int shotcount, const x16vec3_t *dir, const x16 spread_x, const x16 spread_y) {
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

static qboolean can_damage(edict_t *self, edict_t *targ, edict_t *inflictor) {
  x32vec3_t end;
  const trace_t *trace;

  // bmodels need special checking because their origin is 0, 0, 0
  if (targ->v.movetype == MOVETYPE_PUSH) {
    end.x = (targ->v.absmin.x + targ->v.absmax.x) >> 1;
    end.y = (targ->v.absmin.y + targ->v.absmax.y) >> 1;
    end.z = (targ->v.absmin.z + targ->v.absmax.z) >> 1;
    trace = utl_traceline(&inflictor->v.origin, &end, true, self);
    return (trace->fraction == ONE) || (trace->ent == targ);
  }

  trace = utl_traceline(&inflictor->v.origin, &targ->v.origin, true, self);
  if (trace->fraction == ONE)
    return true;

  // no direct LOS, check corners
  // TODO: how much does this matter? seems a bit wasteful

  end.x = targ->v.origin.x + TO_FIX32(15);
  end.y = targ->v.origin.y + TO_FIX32(15);
  end.z = targ->v.origin.z;
  trace = utl_traceline(&inflictor->v.origin, &end, true, self);
  if (trace->fraction == ONE)
    return true;

  end.x = targ->v.origin.x - TO_FIX32(15);
  end.y = targ->v.origin.y - TO_FIX32(15);
  trace = utl_traceline(&inflictor->v.origin, &end, true, self);
  if (trace->fraction == ONE)
    return true;

  end.x = targ->v.origin.x - TO_FIX32(15);
  end.y = targ->v.origin.y + TO_FIX32(15);
  trace = utl_traceline(&inflictor->v.origin, &end, true, self);
  if (trace->fraction == ONE)
    return true;

  end.x = targ->v.origin.x + TO_FIX32(15);
  end.y = targ->v.origin.y - TO_FIX32(15);
  trace = utl_traceline(&inflictor->v.origin, &end, true, self);
  if (trace->fraction == ONE)
    return true;

  return false;
}

void utl_radius_damage(edict_t *inflictor, edict_t *attacker, const s16 damage, edict_t *ignore) {
  x32vec3_t org, delta;

  edict_t *head = G_FindInRadius(&inflictor->v.origin, damage + 40);

  for (; head && head != gs.edicts; head = head->v.chain) {
    if (head == ignore || (head->v.flags & FL_TAKEDAMAGE) == 0)
      continue;

    org.x = head->v.origin.x + ((head->v.mins.x + head->v.maxs.x) >> 1);
    org.y = head->v.origin.y + ((head->v.mins.y + head->v.maxs.y) >> 1);
    org.z = head->v.origin.z + ((head->v.mins.z + head->v.maxs.z) >> 1);

    XVecSub(&inflictor->v.origin, &org, &delta);

    s32 points = XVecLengthIntL(&delta);
    if (points < 0)
      points = 0;

    points = damage - points;

    if (head == attacker)
      points >>= 1;

    if (points < 1)
      continue;

    if (can_damage(attacker, head, inflictor)) {
      // shambler takes half damage from all explosions
      if (head->v.classname == ENT_MONSTER_SHAMBLER)
        points >>= 1;
      utl_damage(head, inflictor, attacker, points);
    }
  }
}

void utl_become_explosion(edict_t *self) {
  self->v.movetype = MOVETYPE_NONE;
  self->v.velocity.x = 0;
  self->v.velocity.y = 0;
  self->v.velocity.z = 0;
  self->v.touch = null_touch;
  self->v.think = utl_remove;
  self->v.nextthink = gs.time + FTOX(0.6);
  self->v.solid = SOLID_NOT;
  G_SetModel(self, 0);
  // TODO: sprite
  fx_spawn_explosion(&self->v.origin);
}

void utl_aim(edict_t *self, x16vec3_t *result) {
  const trace_t *trace;

#if !PLAYER_AIM_ENABLED
  // TODO: setting
  *result = pr.v_forward;
  return;
#endif

  x32vec3_t start;
  start.x = self->v.origin.x;
  start.y = self->v.origin.y;
  start.z = self->v.origin.z + TO_FIX32(20);

  x32vec3_t end;
  end.x = start.x + (x32)pr.v_forward.x * 2048;
  end.y = start.y + (x32)pr.v_forward.y * 2048;
  end.z = start.z + (x32)pr.v_forward.z * 2048;

  // try sending a trace straight
  trace = utl_traceline(&start, &end, false, self);
  if (trace->ent && (trace->ent->v.flags & (FL_TAKEDAMAGE | FL_AUTOAIM)) == (FL_TAKEDAMAGE | FL_AUTOAIM)) {
    *result = pr.v_forward;
    return;
  }

  // try all possible entities that were visible last frame
  x32 dist;
  x32 bestdist = PLAYER_AIM_MINDIST;
  x32 sqrdist;
  x32vec3_t delta;
  x16vec3_t dir;
  x16vec3_t bestdir;
  edict_t *check = gs.edicts + 1;
  edict_t *bestent = NULL;
  for (int i = 1; i <= gs.max_edict; ++i, ++check) {
    if (check == self || (check->v.flags & (FL_TAKEDAMAGE | FL_AUTOAIM | FL_VISIBLE)) != (FL_TAKEDAMAGE | FL_AUTOAIM | FL_VISIBLE))
      continue;

    end.x = check->v.origin.x + ((check->v.mins.x + check->v.maxs.x) >> 1);
    end.y = check->v.origin.y + ((check->v.mins.y + check->v.maxs.y) >> 1);
    end.z = check->v.origin.z + ((check->v.mins.z + check->v.maxs.z) >> 1);

    XVecSub(&end, &start, &delta);
    XVecNormLS(&delta, &dir, &sqrdist);

    dist = XVecDotSS(&dir, &pr.v_forward);
    if (dist < bestdist)
      continue;

    trace = utl_traceline(&start, &end, false, self);
    if (trace->ent == check) {
      // can shoot at this one
      bestdist = dist;
      bestent = check;
      bestdir = dir;
    }
  }

  if (bestent) {
    // uncomment this for vertical-only autoaim
    // XVecSub(&bestent->v.origin, &self->v.origin, &delta);
    // dist = XVecDotSL(&pr.v_forward, &delta);
    // delta.x = xmul32(pr.v_forward.x, dist);
    // delta.y = xmul32(pr.v_forward.y, dist);
    // XVecNormLS(&delta, result, &sqrdist);
    *result = bestdir;
  } else {
    *result = pr.v_forward;
  }
}

qboolean utl_heal(edict_t *e, const s16 amount, const qboolean ignore_max) {
  if (e->v.health <= 0)
    return false;

  if (!ignore_max && e->v.health >= e->v.max_health)
    return false;

  e->v.health += amount;
  if (!ignore_max && e->v.health >= e->v.max_health)
    e->v.health = e->v.max_health;

  if (e->v.health > 250)
    e->v.health = 250;

  return true;
}

static void spike_touch(edict_t *self, edict_t *other) {
  if (other == self->v.owner || other->v.solid == SOLID_TRIGGER)
    return;

  // TODO: sky

  // hit something that bleeds
  if (other->v.flags & FL_TAKEDAMAGE) {
    fx_spawn_blood(&self->v.origin, self->v.dmg);
    utl_damage(other, self, self->v.owner, self->v.dmg);
  } else {
    fx_spawn_gunshot(&self->v.origin);
  }

  utl_remove(self);
}

edict_t *utl_launch_spike(edict_t *self, const x32vec3_t *org, const x16vec3_t *angles, const x16vec3_t *dir, const s16 speed) {
  edict_t *newmis = ED_Alloc();
  newmis->v.owner = self;
  newmis->v.movetype = MOVETYPE_FLYMISSILE;
  newmis->v.solid = SOLID_BBOX;
  newmis->v.angles = *angles;
  newmis->v.touch = spike_touch;
  newmis->v.think = utl_remove;
  newmis->v.nextthink = gs.time + TO_FIX32(6);
  newmis->v.origin = *org;
  newmis->v.velocity.x = (x32)dir->x * speed;
  newmis->v.velocity.y = (x32)dir->y * speed;
  newmis->v.velocity.z = (x32)dir->z * speed;
  newmis->v.dmg = 9;
  G_SetModel(newmis, MDLID_SPIKE);
  G_SetSize(newmis, &x32vec3_origin, &x32vec3_origin);
  G_LinkEdict(newmis, false);
}
