#include "prcommon.h"
#include "monster.h"

static void monster_knight_think(edict_t *self) {
  cycler_think(self, 0, 8);

  // HACK
  if (self->v.viewheight++ >= 64) {
    Snd_StartSoundId(self - gs.edicts, CHAN_VOICE, SFXID_KNIGHT_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
    self->v.viewheight = rand() & 0x1F;
  }
}

void spawn_monster_knight(edict_t *self) {
  G_SetModel(self, MDLID_KNIGHT);
  XVecSetInt(&self->v.mins, -16, -16, -24);
  XVecSetInt(&self->v.maxs, +16, +16, +24);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_knight_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
