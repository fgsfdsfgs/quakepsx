#include "prcommon.h"
#include "monster.h"

static void monster_army_think(edict_t *self) {
  cycler_think(self, 0, 7);

  // HACK
  if (self->v.viewheight++ >= 64) {
    Snd_StartSoundId(self - gs.edicts, CHAN_VOICE, SFXID_SOLDIER_IDLE, &self->v.origin, SND_MAXVOL / 2, ATTN_IDLE);
    self->v.viewheight = rand() & 0x1F;
  }
}

void spawn_monster_army(edict_t *self) {
  G_SetModel(self, MDLID_SOLDIER);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +24);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_army_think;
  self->v.nextthink = gs.time + 410;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
