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

void utl_vectoangles(const x16vec3_t *dir) {
  x16 yaw, pitch;
  x16 forward;

  if (dir->x == 0 && dir->y == 0) {
    yaw = 0;
    if (dir->z > 0)
      pitch = TO_DEG16(90);
    else
      pitch = TO_DEG16(270);
  } else {
    yaw = qatan2(dir->y, dir->x);
    forward = SquareRoot12(XMUL16(dir->x, dir->x) + XMUL16(dir->y, dir->y));
    pitch = qatan2(dir->z, forward);
  }

  pr.v_angles.x = pitch;
  pr.v_angles.y = yaw;
  pr.v_angles.z = 0;
}

void utl_remove(edict_t *self) {
  ED_Free(self);
}

void utl_remove_delayed(edict_t *self) {
  self->v.think = utl_remove;
  self->v.nextthink = gs.time + 1;
}

// called on map change
void Progs_NewMap(void) {
  memset(&pr, 0, sizeof(pr));
}
