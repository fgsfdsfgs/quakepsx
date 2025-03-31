#include "prcommon.h"
#include "monster.h"

static void monster_dog_think(edict_t *self) {
  cycler_think(self, 70, 77);

  // HACK
  if (self->v.viewheight++ >= 64) {
    Snd_StartSoundId(self - gs.edicts, CHAN_VOICE, SFXID_DOG_IDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
    self->v.viewheight = rand() & 0x1F;
  }
}

void spawn_monster_dog(edict_t *self) {
  G_SetModel(self, MDLID_DOG);
  XVecSetInt(&self->v.mins, -32, -32, -24);
  XVecSetInt(&self->v.maxs, +32, +32, +40);
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_dog_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
