#include "common.h"
#include "model.h"
#include "memory.h"
#include "move.h"
#include "game.h"
#include "entity.h"

edict_t *ED_Alloc(u8 classname)
{
  return NULL;
}

void ED_Free(edict_t *ed)
{
  G_UnlinkEdict(ed);
  ed->free = true;
  ed->v.model = NULL;
  ed->v.modelnum = 0;
  ed->v.flags &= ~FL_TAKEDAMAGE;
  ed->v.frame = 0;
  ed->v.origin = x32vec3_origin;
  ed->v.angles = x16vec3_origin;
  ed->v.nextthink = -1;
  ed->v.solid = SOLID_NOT;
  ed->freetime = gs.time;
}
