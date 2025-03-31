#include "prcommon.h"

static inline void spawn_ambient(edict_t *self, const s16 sfxid, const s16 vol) {
  Snd_StaticSoundId(sfxid, &self->v.origin, vol, ATTN_STATIC);
}

void spawn_ambient_comp_hum(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_COMP1, SND_MAXVOL);
}

void spawn_ambient_drip(edict_t *self) {
  // spawn_ambient(self, SFXID_AMBIENCE_DRIP1, SND_MAXVOL >> 1);
}

void spawn_ambient_drone(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_DRONE6, SND_MAXVOL >> 1);
}

void spawn_ambient_suck_wind(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_SUCK1, SND_MAXVOL);
}

void spawn_ambient_swamp1(edict_t *self) {
  spawn_ambient(self, SFXID_AMBIENCE_SWAMP1, SND_MAXVOL >> 1);
}

void spawn_ambient_swamp2(edict_t *self) {
  // SWAMP2 is basically the same as SWAMP1
  spawn_ambient(self, SFXID_AMBIENCE_SWAMP1, SND_MAXVOL >> 1);
}
