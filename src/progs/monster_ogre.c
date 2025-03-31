#include "prcommon.h"
#include "monster.h"

static void monster_ogre_think(edict_t *self) {
  cycler_think(self, 0, 8);

  // HACK
  if (self->v.viewheight++ >= 64) {
    Snd_StartSoundId(self - gs.edicts, CHAN_VOICE, SFXID_OGRE_OGIDLE, &self->v.origin, SND_MAXVOL, ATTN_IDLE);
    self->v.viewheight = rand() & 0x1F;
  }
}

void spawn_monster_ogre(edict_t *self) {
  G_SetModel(self, MDLID_OGRE);
  self->v.mins = gs.worldmodel->hulls[2].mins;
  self->v.maxs = gs.worldmodel->hulls[2].maxs;
  G_SetSize(self, &self->v.mins, &self->v.maxs);
  self->v.think = monster_ogre_think;
  self->v.nextthink = gs.time + 41;
  self->v.solid = SOLID_SLIDEBOX;
  self->v.movetype = MOVETYPE_STEP;
  self->v.flags = FL_MONSTER;
}
