#include <string.h>
#include "prcommon.h"

pr_globals_t pr;

void null_think(edict_t *self) {

}

void null_touch(edict_t *self, edict_t *other) {

}

const trace_t *utl_traceline(const x32vec3_t *v1, const x32vec3_t *v2, const qboolean nomonsters, edict_t *ent) {
  pr.trace = G_Move(v1, &x32vec3_origin, &x32vec3_origin, v2, nomonsters, ent);
  return pr.trace;
}

void utl_makevectors(const x16vec3_t *angles) {
  AngleVectors(angles, &pr.v_forward, &pr.v_right, &pr.v_up);
}

void utl_vectoangles(const x16vec3_t *dir) {
  x16 yaw, pitch;
  x16 forward;

  if (dir->x == 0 && dir->y == 0) {
    yaw = 0;
    if (dir->z > 0)
      pitch = TO_DEG16(90);
    else
      pitch = TO_DEG16(270);
  } else {
    yaw = qatan2(dir->y, dir->x);
    forward = SquareRoot12(XMUL16(dir->x, dir->x) + XMUL16(dir->y, dir->y));
    pitch = qatan2(dir->z, forward);
  }

  pr.v_angles.x = pitch;
  pr.v_angles.y = yaw;
  pr.v_angles.z = 0;
}

void utl_remove(edict_t *self) {
  ED_Free(self);
}

void utl_remove_delayed(edict_t *self) {
  self->v.think = utl_remove;
  self->v.nextthink = gs.time + 1;
}

void utl_set_movedir(edict_t *self, x16vec3_t *movedir) {
  if (self->v.angles.x == 0 && self->v.angles.z == 0) {
    if (self->v.angles.y == -1) {
      // up
      movedir->x = 0;
      movedir->y = 0;
      movedir->z = ONE;
      return;
    } else if (self->v.angles.y == -2) {
      // down
      movedir->x = 0;
      movedir->y = 0;
      movedir->z = -ONE;
      return;
    }
  }

  utl_makevectors(&self->v.angles);
  *movedir = pr.v_forward;
}

static void calc_move_done(edict_t *self) {
  self->v.origin = self->v.door->dest;
  XVecZero(&self->v.velocity);
  G_LinkEdict(self, false);

  self->v.nextthink = -1;
  if (self->v.door->reached)
    self->v.door->reached(self);
}

void utl_calc_move(edict_t *self, const x32vec3_t *tdest, const s16 tspeed, think_fn_t func) {
  self->v.door->reached = func;
  self->v.door->dest = *tdest;
  self->v.think = calc_move_done;

  if (tdest->x == self->v.origin.x && tdest->y == self->v.origin.y && tdest->z == self->v.origin.z) {
    XVecZero(&self->v.velocity);
    self->v.nextthink = self->v.ltime + PR_FRAMETIME;
    return;
  }

  // divide delta length by speed to get time to reach dest
  x32vec3_t vdestdelta;
  XVecSub(tdest, &self->v.origin, &vdestdelta);
  const s32 len = XVecLengthIntL(&vdestdelta);
  const s32 traveltime = xdiv32(len, tspeed);
  if (traveltime < PR_FRAMETIME) {
    XVecZero(&self->v.velocity);
    self->v.nextthink = self->v.ltime + PR_FRAMETIME;
    return;
  }

  // set nextthink to trigger a think when dest is reached
  self->v.nextthink = self->v.ltime + traveltime;

  // scale the destdelta vector by the time spent traveling to get velocity
  const x32 recip = xdiv32(ONE, traveltime);
  self->v.velocity.x = xmul32(recip, vdestdelta.x);
  self->v.velocity.y = xmul32(recip, vdestdelta.y);
  self->v.velocity.z = xmul32(recip, vdestdelta.z);
}

static void delay_think(edict_t *self) {
  utl_usetargets(self, self->v.extra_ptr);
  utl_remove(self);
}

void utl_usetargets(edict_t *self, edict_t *activator) {
  // check for a delay
  if (self->v.delay) {
    // create a temp object to fire at a later time
    edict_t *t = ED_Alloc();
    t->v.classname = ENT_DELAYED_USE;
    t->v.nextthink = gs.time + self->v.delay;
    t->v.think = delay_think;
    t->v.owner = self;
    t->v.target = self->v.target;
    t->v.killtarget = self->v.killtarget;
    t->v.extra_trigger.string = self->v.extra_trigger.string;
    t->v.extra_ptr = activator;
    return;
  }

  // print the string
  const u16 msg = self->v.extra_trigger.string;
  if (activator->v.classname == ENT_PLAYER && (msg || self->v.classname == ENT_TRIGGER_SECRET)) {
    Scr_SetCenterMsg(msg ? (gs.worldmodel->strings + msg) : "You found a secret area!");
    if (!self->v.noise)
      utl_sound(activator, CHAN_VOICE, SFXID_MISC_TALK, SND_MAXVOL, ATTN_NORM);
  }

  // kill the killtargets
  if (self->v.killtarget) {
    edict_t *t = G_FindByTargetname(gs.edicts, self->v.killtarget);
    while (t != gs.edicts) {
      utl_remove(t);
      t = G_FindByTargetname(t, self->v.killtarget);
    }
  }

  // fire targets
  if (self->v.target) {
    edict_t *t = G_FindByTargetname(gs.edicts, self->v.target);
    while (t != gs.edicts) {
      if (t->v.use)
        t->v.use(t, activator);
      t = G_FindByTargetname(t, self->v.target);
    }
  }
}

void utl_sound(edict_t *self, const s16 chan, const s16 sndid, const u8 vol, const x32 attn) {
  x32vec3_t org;
  org.x = self->v.origin.x + ((self->v.mins.x + self->v.maxs.x) >> 1);
  org.y = self->v.origin.y + ((self->v.mins.y + self->v.maxs.y) >> 1);
  org.z = self->v.origin.z + ((self->v.mins.z + self->v.maxs.z) >> 1);
  Snd_StartSoundId(EDICT_NUM(self), chan, sndid, &org, vol, attn);
}

// called on map change
void Progs_NewMap(void) {
  memset(&pr, 0, sizeof(pr));
}
