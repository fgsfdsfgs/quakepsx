#include <string.h>
#include "prcommon.h"

pr_globals_t pr;

void null_think(edict_t *self) {

}

void null_touch(edict_t *self, edict_t *other) {

}

const trace_t *utl_traceline(x32vec3_t *v1, x32vec3_t *v2, const qboolean nomonsters, edict_t *ent) {
  pr.trace = G_Move(v1, &x32vec3_origin, &x32vec3_origin, v2, nomonsters, ent);
  return pr.trace;
}

void utl_makevectors(const x16vec3_t *angles) {
  AngleVectors(angles, &pr.v_forward, &pr.v_right, &pr.v_up);
}

// called on map change
void Progs_NewMap(void) {
  memset(&pr, 0, sizeof(pr));
}
