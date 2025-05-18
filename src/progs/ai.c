#include "prcommon.h"

void ai_changeyaw(edict_t *self) {
  x16 ideal, current, move, speed;

  current = self->v.angles.y & (ONE - 1);
  ideal = self->v.monster->ideal_yaw;
  speed = self->v.monster->yaw_speed;

  if (current == ideal)
    return;

  move = ideal - current;
  if (ideal > current) {
    if (move >= TO_DEG16(180))
      move = move - ONE;
  } else {
    if (move <= -TO_DEG16(180))
      move = move + ONE;
  }

  if (move > 0) {
    if (move > speed)
      move = speed;
  } else {
    if (move < -speed)
      move = -speed;
  }

  self->v.angles.y = (current + move) & (ONE - 1);
}

qboolean ai_movestep(edict_t *ent, const x32vec3_t *move, const qboolean relink) {
  x32vec3_t oldorg;
  x32vec3_t neworg;
  x32vec3_t end;
  edict_t *enemy;
  const trace_t *trace;
  int i;

  // try the move
  oldorg = ent->v.origin;
  XVecAdd(&ent->v.origin, move, &neworg);

  // flying and swimming monsters don't step up
  if (ent->v.flags & (FL_SWIM | FL_FLY)) {
    // try one move with vertical motion, then one without
    for (i = 0; i < 2; i++) {
      XVecAdd(&ent->v.origin, move, &neworg);
      enemy = ent->v.monster->enemy;
      if (i == 0 && enemy != gs.edicts) {
        const x32 dz = ent->v.origin.z - enemy->v.origin.z;
        if (dz > TO_FIX32(40))
          neworg.z -= TO_FIX32(8);
        if (dz < TO_FIX32(30))
          neworg.z += TO_FIX32(8);
      }
      // check move
      trace = G_Move(&ent->v.origin, &ent->v.mins, &ent->v.maxs, &neworg, false, ent);
      if (trace->fraction == ONE) {
        if ((ent->v.flags & FL_SWIM) && G_PointContents(&trace->endpos) == CONTENTS_EMPTY)
          return false; // swim monster left water
        ent->v.origin = trace->endpos;
        if (relink)
          G_LinkEdict(ent, true);
        return true;
      }
      if (enemy == gs.edicts)
        break;
    }
    // couldn't move
    return false;
  }

  // push down from a step height above the wished position
  neworg.z += G_STEPSIZE;
  end = neworg;
  end.z -= 2*G_STEPSIZE;

  trace = G_Move(&neworg, &ent->v.mins, &ent->v.maxs, &end, false, ent);

  if (trace->allsolid)
    return false;

  if (trace->startsolid) {
    neworg.z -= G_STEPSIZE;
    trace = G_Move(&neworg, &ent->v.mins, &ent->v.maxs, &end, false, ent);
    if (trace->allsolid || trace->startsolid)
      return false;
  }

  if (trace->fraction == ONE) {
    // if monster had the ground pulled out, go ahead and fall
    if (ent->v.flags & FL_PARTIALGROUND) {
      XVecAdd(&ent->v.origin, move, &ent->v.origin);
      if (relink)
        G_LinkEdict(ent, true);
      ent->v.flags &= ~FL_ONGROUND;
      return true;
    }
    // walked off an edge
    return false;
  }

  // check point traces down for dangling corners
  ent->v.origin = trace->endpos;

  if (!G_CheckBottom(ent)) {
    if (ent->v.flags & FL_PARTIALGROUND) {
      // entity had floor mostly pulled out from underneath it
      // and is trying to correct
      if (relink)
        G_LinkEdict(ent, true);
      return true;
    }
    ent->v.origin = oldorg;
    return false;
  }

  if (ent->v.flags & FL_PARTIALGROUND)
    ent->v.flags &= ~FL_PARTIALGROUND;

  ent->v.groundentity = trace->ent;

  // the move is ok
  if (relink)
    G_LinkEdict(ent, true);
  return true;
}

qboolean ai_walkmove(edict_t *ent, x16 yaw, const x32 dist) {
  if ((ent->v.flags & (FL_ONGROUND | FL_SWIM | FL_FLY)) == 0)
    return false;

  x32vec3_t move;
  move.x = xmul32(ccos(yaw), dist);
  move.y = xmul32(csin(yaw), dist);
  move.z = 0;

  return ai_movestep(ent, &move, true);
}

qboolean ai_step_direction(edict_t *ent, x16 yaw, const x32 dist) {
  ent->v.monster->ideal_yaw = yaw;
  ai_changeyaw(ent);

  x32vec3_t move;
  move.x = xmul32(ccos(yaw), dist);
  move.y = xmul32(csin(yaw), dist);
  move.z = 0;

  x32vec3_t oldorigin = ent->v.origin;
  if (ai_movestep(ent, &move, false)) {
    const x16 delta = ent->v.angles.y - ent->v.monster->ideal_yaw;
    if (delta > TO_DEG16(45) && delta < TO_DEG16(315))
      ent->v.origin = oldorigin; // not turned far enough, so don't take the step
    G_LinkEdict(ent, true);
    return true;
  }

  G_LinkEdict(ent, true);
  return false;
}

qboolean ai_close_enough(edict_t *ent, edict_t *goal, const x32 dist) {
  for (int i = 0; i < 3; ++i) {
    if (goal->v.absmin.d[i] > ent->v.absmax.d[i] + dist)
      return false;
    if (goal->v.absmax.d[i] < ent->v.absmin.d[i] - dist)
      return false;
  }
  return true;
}

void ai_new_chase_dir(edict_t *actor, edict_t *enemy, const x32 dist) {
  const x16 olddir = ((actor->v.monster->ideal_yaw / 512) * 512) & (ONE - 1);
  const x16 turnaround = (olddir - TO_DEG16(180)) & (ONE - 1);

  x16 d[3];
  x16 tdir;
  const x32 deltax = enemy->v.origin.x - actor->v.origin.x;
  const x32 deltay = enemy->v.origin.y - actor->v.origin.y;

  if (deltax > TO_FIX32(10))
    d[1] = 0;
  else if (deltax < -TO_FIX32(10))
    d[1] = TO_DEG16(180);
  else
    d[1] = -1;

  if (deltay < -TO_FIX32(10))
    d[2] = TO_DEG16(270);
  else if (deltay > TO_FIX32(10))
    d[2] = TO_DEG16(90);
  else
    d[2] = -1;

  // try direct route
  if (d[1] != -1 && d[2] != -1) {
    if (d[1] == 0)
      tdir = d[2] == TO_DEG16(90) ? TO_DEG16(45) : TO_DEG16(315);
    else
      tdir = d[2] == TO_DEG16(90) ? TO_DEG16(135) : TO_DEG16(215);
    if (tdir != turnaround && ai_step_direction(actor, tdir, dist))
      return;
  }

  // try other directions
  if (((rand() & 3) & 1) || abs(deltay) > abs(deltax)) {
    tdir = d[1];
    d[1] = d[2];
    d[2] = tdir;
  }

  if (d[1] != -1 && d[1] != turnaround && ai_step_direction(actor, d[1], dist))
    return;

  if (d[2] != -1 && d[2] != turnaround && ai_step_direction(actor, d[2], dist))
    return;

  // there is no direct path to the player, so pick another direction
  if (olddir != -1 && ai_step_direction(actor, olddir, dist))
    return;

  // randomly determine direction of search
  if (rand() & 1) {
    for (tdir = 0; tdir <= TO_DEG16(315); tdir += TO_DEG16(45))
      if (tdir != turnaround && ai_step_direction(actor, tdir, dist))
        return;
  } else {
    for (tdir = TO_DEG16(315); tdir >= 0; tdir -= TO_DEG16(45))
      if (tdir != turnaround && ai_step_direction(actor, tdir, dist))
        return;
  }

  if (turnaround != -1 && ai_step_direction(actor, turnaround, dist))
    return;

  actor->v.monster->ideal_yaw = olddir; // can't move

  // if a bridge was pulled out from underneath a monster, it may not have
  // a valid standing position at all
  if (!G_CheckBottom(actor))
    actor->v.flags |= FL_PARTIALGROUND;
}

void ai_movetogoal(edict_t *ent, const x32 dist) {
  if ((ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)) == 0)
    return;

  edict_t *goal = ent->v.monster->goalentity;

  if (ent->v.monster->enemy != gs.edicts && ai_close_enough(ent, goal, dist))
    return;

  // bump around...
  if ((rand() & 3) == 1 || !ai_step_direction(ent, ent->v.monster->ideal_yaw, dist))
    ai_new_chase_dir(ent, goal, dist);
}

/*
  in nightmare mode, all attack_finished times become 0
  some monsters refire twice automatically
*/
void ai_attack_finished(edict_t *self, const x32 dt) {
  self->v.count = 0; // refire count for nightmare
  if (gs.skill != 3)
    self->v.monster->attack_finished = gs.time + dt;
}

void ai_hunt_target(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  mon->goalentity = mon->enemy;
  mon->ideal_yaw = VecDeltaToYaw(&mon->goalentity->v.origin, &self->v.origin);

  monster_set_next_state(self, MSTATE_RUN);

  ai_attack_finished(self, ONE); // wait a while before first attack
}

s32 ai_range(edict_t *self, edict_t *targ) {
  const x32vec3_t spot1 = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + self->v.viewheight }};
  const x32vec3_t spot2 = {{ targ->v.origin.x, targ->v.origin.y, targ->v.origin.z + targ->v.viewheight }};
  const x32vec3_t delta = {{ spot2.x - spot1.x, spot2.y - spot1.y, spot2.z - spot1.z }};
  const s32 sqrlen = XVecLengthSqrIntL(&delta);
  if (sqrlen < 100 * 100)
    return RANGE_MELEE;
  if (sqrlen < 500 * 500)
    return RANGE_NEAR;
  if (sqrlen < 1000 * 1000)
    return RANGE_MID;
  return RANGE_FAR;
}

qboolean ai_visible(edict_t *self, edict_t *targ) {
  x32vec3_t spot1 = {{ self->v.origin.x, self->v.origin.y, self->v.origin.z + self->v.viewheight }};
  x32vec3_t spot2 = {{ targ->v.origin.x, targ->v.origin.y, targ->v.origin.z + targ->v.viewheight }};

  const trace_t *trace = utl_traceline(&spot1, &spot2, true, self);

  if (trace->inopen && trace->inwater)
    return false; // sight line crossed contents

  if (trace->fraction == ONE)
    return true;

  return false;
}

qboolean ai_infront(edict_t *self, edict_t *targ) {
  x32vec3_t delta;
  x16vec3_t vec;
  x32 delta_sqrlen;
  utl_makevectors(&self->v.angles);
  XVecSub(&targ->v.origin, &self->v.origin, &delta);
  XVecNormLS(&delta, &vec, &delta_sqrlen);
  const x32 dot = XVecDotSS(&vec, &pr.v_forward);
  return (dot > FTOX(0.3));
}

void ai_foundtarget(edict_t *self) {
  monster_fields_t *mon = self->v.monster;

  if (mon->enemy->v.classname == ENT_PLAYER) {
    // let other monsters see this monster for a while
    pr.sight_entity = self;
    pr.sight_entity_time = gs.time;
  }

  mon->show_hostile = gs.time + ONE; // wake up other monsters

  if (mon->class->sight_sound)
    utl_sound(self, CHAN_VOICE, mon->class->sight_sound, SND_MAXVOL, ATTN_NORM);

  ai_hunt_target(self);
}

qboolean ai_findtarget(edict_t *self) {
  monster_fields_t *self_mon = self->v.monster;

  // if the first spawnflag bit is set, the monster will only wake up on
  // really seeing the player, not another monster getting angry

  // spawnflags & 3 is a big hack, because zombie crucified used the first
  // spawn flag prior to the ambush flag, and I forgot about it, so the second
  // spawn flag works as well
  edict_t *target;
  if (pr.sight_entity && pr.sight_entity_time >= (gs.time - PR_FRAMETIME) && !(self->v.spawnflags & 3)) {
    target = pr.sight_entity;
    if (target->v.flags & FL_MONSTER) {
      if (target->v.monster->enemy == self_mon->enemy)
        return false;
    }
  } else {
    target = G_CheckClient(self);
    if (!target)
      return false; // current check entity isn't in PVS
  }

  if (target == self_mon->enemy)
    return false;
  if (target->v.flags & FL_NOTARGET)
    return false;
  if (target->v.classname == ENT_PLAYER && (target->v.player->stats.items & IT_INVISIBILITY))
    return false;

  const s32 r = ai_range(self, target);
  if (r == RANGE_FAR)
    return false;

  if (!ai_visible(self, target))
    return false;

  if (r == RANGE_NEAR) {
    const x32 show_hostile = target->v.classname == ENT_PLAYER ?
      target->v.player->show_hostile : target->v.monster->show_hostile;
    if (show_hostile < gs.time + PR_FRAMETIME && !ai_infront(self, target))
      return false;
  } else if (r == RANGE_MID) {
    if (!ai_infront(self, target))
      return false;
  }

  // got one
  self_mon->enemy = target;
  if (target->v.classname != ENT_PLAYER && (target->v.flags & FL_MONSTER)) {
    self_mon->enemy = target->v.monster->enemy;
    if (self_mon->enemy->v.classname != ENT_PLAYER) {
      self_mon->enemy = gs.edicts;
      return false;
    }
  }

  ai_foundtarget(self);

  return true;
}

static qboolean ai_check_attack_generic(edict_t *self) {
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

  const qboolean have_melee = (mon->class->state_table[MSTATE_MELEE].think_fn != NULL);
  if (pr.enemy_range == RANGE_MELEE && have_melee) {
    // melee attack
    monster_exec_state(self, MSTATE_MELEE);
    return true;
  }

  // missile attack
  if (!mon->class->state_table[MSTATE_MISSILE].think_fn)
    return false;

  if (pr.enemy_range == RANGE_FAR || mon->attack_finished > gs.time)
    return false;

  x32 chance;
  switch (pr.enemy_range) {
  case RANGE_MELEE:
    chance = FTOX(0.9);
    mon->attack_finished = 0;
    break;
  case RANGE_NEAR:
    chance = have_melee ? FTOX(0.2) : FTOX(0.4);
    break;
  case RANGE_MID:
    chance = have_melee ? FTOX(0.05) : FTOX(0.1);
    break;
  default:
    chance = 0;
    break;
  }

  if (xrand32() < chance) {
    monster_exec_state(self, MSTATE_MISSILE);
    ai_attack_finished(self, xrand32() << 1);
    return true;
  }

  return false;
}

qboolean ai_check_any_attack(edict_t *self) {
  if (!pr.enemy_vis)
    return false;

  if (self->v.monster->class->fn_check_attack)
    return self->v.monster->class->fn_check_attack(self);

  return ai_check_attack_generic(self);
}

qboolean ai_facing_ideal(edict_t *self) {
  const x16 delta = (self->v.angles.y - self->v.monster->ideal_yaw) & (ONE - 1);
  if (delta > TO_DEG16(45) && delta < TO_DEG16(315))
    return false;
  return true;
}

void ai_stand(edict_t *self) {
  if (ai_findtarget(self))
    return;
  if (gs.time > self->v.monster->pause_time)
    monster_exec_state(self, MSTATE_WALK);
}

void ai_turn(edict_t *self) {
  if (ai_findtarget(self))
    return;
  ai_changeyaw(self);
}

void ai_face(edict_t *self) {
  self->v.monster->ideal_yaw = VecDeltaToYaw(&self->v.monster->enemy->v.origin, &self->v.origin);
  ai_changeyaw(self);
}

void ai_walk(edict_t *self, const x32 dist) {
  if (ai_findtarget(self))
    return;
  ai_movetogoal(self, dist);
}

static inline void ai_run_slide(edict_t *self, const x32 movedist) {
  // strafe sideways, but stay at approximately the same range
  monster_fields_t *mon = self->v.monster;

  mon->ideal_yaw = pr.enemy_yaw;
  ai_changeyaw(self);

  const x16 ofs = (mon->lefty) ? TO_DEG16(90) : TO_DEG16(-90);
  if (ai_walkmove(self, mon->ideal_yaw + ofs, movedist))
    return;

  mon->lefty = !mon->lefty;

  ai_walkmove(self, mon->ideal_yaw - ofs, movedist);
}

static inline void ai_run_attack(edict_t *self) {
  // turn in place until within an angle to launch an attack
  monster_fields_t *mon = self->v.monster;
  mon->ideal_yaw = pr.enemy_yaw;
  ai_changeyaw(self);
  if (ai_facing_ideal(self)) {
    monster_exec_state(self, mon->attack_state == AS_MISSILE ? MSTATE_MISSILE : MSTATE_MELEE);
    mon->attack_state = AS_STRAIGHT;
  }
}

void ai_run(edict_t *self, const x32 dist) {
  monster_fields_t *mon = self->v.monster;

  // see if the enemy is dead
  if (mon->enemy->v.health <= 0) {
    mon->enemy = gs.edicts;
    if (mon->oldenemy != gs.edicts && mon->oldenemy->v.health > 0) {
      mon->enemy = mon->oldenemy;
      ai_hunt_target(self);
    } else {
      monster_exec_state(self, mon->movetarget != gs.edicts ? MSTATE_WALK : MSTATE_STAND);
      return;
    }
  }

  mon->show_hostile = gs.time + ONE; // wake up other monsters

  // check knowledge of enemy
  pr.enemy_vis = ai_visible(self, mon->enemy);
  if (pr.enemy_vis)
    mon->search_time = gs.time + TO_FIX32(5);

  pr.enemy_infront = ai_infront(self, mon->enemy);
  pr.enemy_range = ai_range(self, mon->enemy);
  pr.enemy_yaw = VecDeltaToYaw(&mon->enemy->v.origin, &self->v.origin);

  if (mon->attack_state == AS_MISSILE || mon->attack_state == AS_MELEE) {
    ai_run_attack(self);
    return;
  }

  if (ai_check_any_attack(self))
    return; // beginning an attack

  if (mon->attack_state == AS_SLIDING) {
    ai_run_slide(self, dist);
    return;
  }

  // head straight in
  ai_movetogoal(self, dist);
}

void ai_charge(edict_t *self, const x32 dist) {
  ai_face(self);
  ai_movetogoal(self, dist);
}

void ai_forward(edict_t *self, const x32 dist) {
  ai_walkmove(self, self->v.angles.y, dist);
}

void ai_back(edict_t *self, const x32 dist) {
  const x16 reverse = (self->v.angles.y + TO_DEG16(180)) & (ONE - 1);
  ai_walkmove(self, reverse, dist);
}

void ai_pain(edict_t *self, const x32 dist) {
  ai_back(self, dist);
}

void ai_painforward(edict_t *self, const x32 dist) {
  ai_walkmove(self, self->v.monster->ideal_yaw, dist);
}

void ai_checkrefire(edict_t *self, const s16 state) {
  if (gs.skill != 3)
    return;
  if (self->v.count == 1)
    return;
  if (!ai_visible(self, self->v.monster->enemy))
    return;
  self->v.count = 1;
  monster_set_next_state(self, state);
}

void ai_gib(edict_t *self) {
  utl_sound(self, CHAN_VOICE, SFXID_PLAYER_GIB, SND_MAXVOL, ATTN_NORM);
  fx_throw_head(self, MDLID_GIB1, self->v.health);
  fx_throw_gib(&self->v.origin, MDLID_GIB2, self->v.health);
  fx_throw_gib(&self->v.origin, MDLID_GIB3, self->v.health);
}
