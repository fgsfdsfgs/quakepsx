#include "prcommon.h"

void cycler_think(edict_t *self, const s16 idle_start, const s16 idle_end) {
  self->v.frame++;

  if (self->v.frame > idle_end)
    self->v.frame = idle_start;

  self->v.nextthink = gs.time + PR_FRAMETIME;
}
