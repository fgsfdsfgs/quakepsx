#include <string.h>
#include "common.h"
#include "model.h"
#include "memory.h"
#include "move.h"
#include "game.h"
#include "system.h"
#include "entity.h"

edict_t *ED_Alloc(void) {
  int i;
  edict_t *e = gs.edicts + 1 + MAX_PLAYERS;
  for (i = 1 + MAX_PLAYERS; i < gs.num_edicts; ++i, ++e) {
    // the first couple seconds of server time can involve a lot of
    // freeing and allocating, so relax the replacement policy
    if (e->free && (e->freetime < FTOX(2.0) || gs.time - e->freetime > FTOX(0.5))) {
      memset(&e->v, 0, sizeof(e->v));
      e->free = false;
      if (i > gs.max_edict)
        gs.max_edict = i;
      return e;
    }
  }

  Sys_Error("ED_Alloc(): no free edicts");

  return NULL;
}

void ED_Free(edict_t *ed) {
  G_UnlinkEdict(ed);
  ed->free = true;
  ed->v.model = NULL;
  ed->v.modelnum = 0;
  ed->v.flags &= ~(FL_TAKEDAMAGE | FL_AUTOAIM);
  ed->v.frame = 0;
  ed->v.origin = x32vec3_origin;
  ed->v.angles = x16vec3_origin;
  ed->v.nextthink = -1;
  ed->v.solid = SOLID_NOT;
  ed->v.touch = NULL;
  ed->v.think = NULL;
  ed->v.use = NULL;
  ed->v.classname = 0xff;
  ed->freetime = gs.time;
}
