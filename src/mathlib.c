#include "mathlib.h"
#include "types.h"
#include "model.h"

const x32vec3_t x32vec3_origin = { 0 };
const x16vec3_t x16vec3_origin = { 0 };

void AngleVectors(const x16vec3_t *angles, x16vec3_t *forward, x16vec3_t *right, x16vec3_t *up) {
  register x16 angle;
  x32 sr, sp, sy, cr, cp, cy;

  angle = angles->d[YAW];
  sy = csin(angle);
  cy = ccos(angle);
  angle = angles->d[PITCH];
  sp = csin(angle);
  cp = ccos(angle);
  angle = angles->d[ROLL];
  sr = csin(angle);
  cr = ccos(angle);

  forward->d[0] = XMUL16(cp, cy);
  forward->d[1] = XMUL16(cp, sy);
  forward->d[2] = -sp;
  right->d[0] = (XMUL16(XMUL16(-sr, sp), cy) + XMUL16(-cr, -sy));
  right->d[1] = (XMUL16(XMUL16(-sr, sp), sy) + XMUL16(-cr, cy));
  right->d[2] = XMUL16(-sr, cp);
  up->d[0] = (XMUL16(XMUL16(cr, sp), cy) + XMUL16(-sr, -sy));
  up->d[1] = (XMUL16(XMUL16(cr, sp), sy) + XMUL16(-sr, cy));
  up->d[2] = XMUL16(cr, cp);
}

int BoxOnPlaneSide(const x32vec3_t *emins, const x32vec3_t *emaxs, const mplane_t *p) {
  x32 dist1, dist2;
  x32vec3_t *v = PSX_SCRATCH;
  int sides;

  // fast axial cases
  if (p->type < 3) {
    if (p->dist <= emins->d[p->type])
      return 1;
    if (p->dist >= emaxs->d[p->type])
      return 2;
    return 3;
  }

  // general case
  switch (p->signbits) {
  case 0:
    v[0] = *emaxs;
    v[1] = *emins;
    break;
  case 1:
    v[0] = (x32vec3_t){ emins->x, emaxs->y, emaxs->z };
    v[1] = (x32vec3_t){ emaxs->x, emins->y, emins->z };
    break;
  case 2:
    v[0] = (x32vec3_t){ emaxs->x, emins->y, emaxs->z };
    v[1] = (x32vec3_t){ emins->x, emaxs->y, emins->z };
    break;
  case 3:
    v[0] = (x32vec3_t){ emins->x, emins->y, emaxs->z };
    v[1] = (x32vec3_t){ emaxs->x, emaxs->y, emins->z };
    break;
  case 4:
    v[0] = (x32vec3_t){ emaxs->x, emaxs->y, emins->z };
    v[1] = (x32vec3_t){ emins->x, emins->y, emaxs->z };
    break;
  case 5:
    v[0] = (x32vec3_t){ emins->x, emaxs->y, emins->z };
    v[1] = (x32vec3_t){ emaxs->x, emins->y, emaxs->z };
    break;
  case 6:
    v[0] = (x32vec3_t){ emaxs->x, emins->y, emins->z };
    v[1] = (x32vec3_t){ emins->x, emaxs->y, emaxs->z };
    break;
  case 7:
    v[0] = *emins;
    v[1] = *emaxs;
    break;
  default:
    // never happens
    v[0].x = v[0].y = v[0].z = 0;
    v[1].x = v[1].y = v[1].z = 0;
    break;
  }

  dist1 = XVecDotSL(&p->normal, &v[0]);
  dist2 = XVecDotSL(&p->normal, &v[1]);

  sides = 0;
  if (dist1 >= p->dist)
    sides = 1;
  if (dist2 < p->dist)
    sides |= 2;

  return sides;
}
