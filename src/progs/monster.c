#include "prcommon.h"
#include "monster.h"

void cycler_think(edict_t *self) {
  self->v.frame++;
  if (self->v.frame >= ((amodel_t *)self->v.model)->numframes)
    self->v.frame = 0;
  self->v.nextthink = gs.time + PR_FRAMETIME;
}
