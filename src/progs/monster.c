#include "prcommon.h"

void monster_init(edict_t *self, const monster_class_t *class) {
  // all monsters are created at map init time in OG Quake, so we can alloc extra fields here
  // to avoid bloating edict_t
  monster_fields_t *mon = Mem_ZeroAlloc(sizeof(monster_fields_t));
  mon->class = class;
  mon->enemy = gs.edicts;
  mon->oldenemy = gs.edicts;
  mon->movetarget = gs.edicts;
  mon->goalentity = gs.edicts;
  mon->state_num = MSTATE_NONE;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.monster = mon;
  self->v.th_die = class->fn_start_die;
  monster_set_next_state(self, MSTATE_STAND);
  pr.total_monsters += 1;
}

static void monster_use(edict_t *self, edict_t *activator) {
  monster_fields_t *mon = self->v.monster;

  if (mon->enemy != gs.edicts)
    return;

  if (self->v.health <= 0)
    return;

  if (activator->v.classname != ENT_PLAYER)
    return;

  if (activator->v.flags & FL_NOTARGET)
    return;

  if (activator->v.player->stats.items & IT_INVISIBILITY)
    return; // player is invisible

  // delay reaction so if the monster is teleported, its sound is still heard
  mon->enemy = activator;
  self->v.nextthink = gs.time + PR_FRAMETIME;
  self->v.think = ai_foundtarget;
}

static inline void monster_start_go(edict_t *self, const x16 default_yawspeed, const x32 viewheight, const qboolean drop) {
  monster_fields_t *mon = self->v.extra_ptr;

  self->v.flags |= FL_MONSTER | FL_TAKEDAMAGE | FL_AUTOAIM;
  self->v.use = monster_use;
  self->v.viewheight = viewheight;

  mon->ideal_yaw = self->v.angles.y;
  if (!mon->yaw_speed)
    mon->yaw_speed = default_yawspeed;

  if (drop) {
    self->v.origin.z += ONE;
    G_DropToFloor(self);
  }

  if (!ai_walkmove(self, 0, 0)) {
    Sys_Printf(
      "Monster %d (%02x) stuck in wall at (%d, %d, %d)\n",
      EDICT_NUM(self), self->v.classname,
      self->v.origin.x >> FIXSHIFT, self->v.origin.y >> FIXSHIFT, self->v.origin.z >> FIXSHIFT
    );
  }

  if (self->v.target) {
    mon->movetarget = mon->goalentity = G_FindByTargetname(gs.edicts, self->v.target);
    if (mon->movetarget == gs.edicts) {
      Sys_Printf("Monster %d (%02x) can't find target %x\n", EDICT_NUM(self), self->v.classname, self->v.target);
    } else {
      mon->ideal_yaw = VecDeltaToYaw(&mon->goalentity->v.origin, &self->v.origin);
      if (mon->movetarget->v.classname == ENT_PATH_CORNER) {
        monster_exec_state(self, MSTATE_WALK);
        return;
      }
    }
  }

  mon->pause_time = TO_FIX32(0xFFFF);
  monster_exec_state(self, MSTATE_STAND);

  // spread think times so they don't all happen at same time
  self->v.nextthink += xrand32() >> 1;
}

static void walkmonster_start_go(edict_t *self) {
  monster_start_go(self, TO_DEG16(20), TO_FIX32(25), true);
}

static void flymonster_start_go(edict_t *self) {
  self->v.flags |= FL_FLY;
  monster_start_go(self, TO_DEG16(10), TO_FIX32(25), false);
}

static void swimmonster_start_go(edict_t *self) {
  monster_fields_t *mon = self->v.extra_ptr;

  self->v.flags |= FL_MONSTER | FL_TAKEDAMAGE | FL_AUTOAIM | FL_SWIM;
  self->v.use = monster_use;
  self->v.viewheight = TO_FIX32(10);

  mon->ideal_yaw = self->v.angles.y;
  if (!mon->yaw_speed)
    mon->yaw_speed = TO_DEG16(10);

  if (self->v.target) {
    mon->goalentity = mon->movetarget = G_FindByTargetname(gs.edicts, self->v.target);
    if (mon->movetarget == gs.edicts) {
      Sys_Printf("Monster %d (%02x) can't find target\n", EDICT_NUM(self), self->v.classname);
    } else {
      mon->ideal_yaw = VecDeltaToYaw(&mon->goalentity->v.origin, &self->v.origin);
      monster_exec_state(self, MSTATE_WALK);
    }
  } else {
    Sys_Printf("mon %02x state %02x\n", EDICT_NUM(self), MSTATE_STAND);
    mon->pause_time = TO_FIX32(0xFFFF);
    monster_exec_state(self, MSTATE_STAND);
  }

  // spread think times so they don't all happen at same time
  self->v.nextthink += xrand32() >> 1;
}

void monster_death_use(edict_t *self) {
  // fall to ground
  self->v.flags &= ~(FL_FLY | FL_SWIM);

  if (!self->v.target)
    return;

  // TODO: SUB_UseTargets
}

void monster_exec_state(edict_t *self, const s16 state) {
  monster_fields_t *mon = self->v.extra_ptr;
  const monster_state_t *s = &mon->class->state_table[state];
  if (s->think_fn) {
    mon->next_frame = s->first_frame;
    s->think_fn(self);
  }
}

void monster_set_next_state(edict_t *self, const s16 state) {
  monster_fields_t *mon = self->v.extra_ptr;
  const monster_state_t *s = &mon->class->state_table[state];
  if (s->think_fn) {
    mon->next_frame = s->first_frame;
    self->v.think = s->think_fn;
    self->v.nextthink = gs.time + PR_FRAMETIME;
  }
}

void monster_looping_state(edict_t *self, const s16 state) {
  monster_fields_t *mon = self->v.extra_ptr;
  if (mon->state_num != state) {
    mon->state_num = state;
    mon->state = &mon->class->state_table[state];
    mon->next_frame = mon->state->first_frame;
    self->v.think = mon->state->think_fn;
  }

  self->v.frame = mon->next_frame;

  if (mon->next_frame >= mon->state->last_frame)
    mon->next_frame = mon->state->first_frame;
  else
    mon->next_frame++;

  self->v.nextthink = gs.time + PR_FRAMETIME;
}

void monster_finite_state(edict_t *self, const s16 state, const s16 next_state) {
  monster_fields_t *mon = self->v.extra_ptr;
  if (mon->state_num != state) {
    mon->state_num = state;
    mon->state = &mon->class->state_table[state];
    mon->next_frame = mon->state->first_frame;
    self->v.think = mon->state->think_fn;
  }

  self->v.frame = mon->next_frame;

  if (mon->next_frame < mon->state->last_frame) {
    mon->next_frame++;
    self->v.nextthink = gs.time + PR_FRAMETIME;
  } else if (next_state >= 0) {
    monster_set_next_state(self, next_state);
  }
}

void walkmonster_start(edict_t *self, const monster_class_t *class) {
  monster_init(self, class);
  // delay drop to floor to make sure all doors have been spawned
  // spread think times so they don't all happen at same time
  self->v.nextthink = self->v.nextthink + (xrand32() >> 1);
  self->v.think = walkmonster_start_go;
}

void flymonster_start(edict_t *self, const monster_class_t *class) {
  monster_init(self, class);
  // spread think times so they don't all happen at same time
  self->v.nextthink = self->v.nextthink + (xrand32() >> 1);
  self->v.think = flymonster_start_go;
}

void swimmonster_start(edict_t *self, const monster_class_t *class) {
  monster_init(self, class);
  // spread think times so they don't all happen at same time
  self->v.nextthink = self->v.nextthink + (xrand32() >> 1);
  self->v.think = swimmonster_start_go;
}

void cycler_think(edict_t *self, const s16 idle_start, const s16 idle_end) {
  self->v.frame++;

  if (self->v.frame > idle_end)
    self->v.frame = idle_start;

  self->v.nextthink = gs.time + PR_FRAMETIME;
}
