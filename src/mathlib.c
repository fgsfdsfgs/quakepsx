#include "mathlib.h"
#include "types.h"
#include "fixed.h"
#include "vector.h"
#include "model.h"

x32vec3_t x32vec3_origin = { 0 };
x16vec3_t x16vec3_origin = { 0 };

// atan2 for an integer XY, returns in the same angle format as everything else (4096 = 360 degrees)
// method spied in Soul Reaver (soul-re)
x16 qatan2(s32 y, s32 x) {
  if (x == 0)
    x = 1;

  if (y == 0)
    return (x < 1) * TO_DEG16(180);

  const s32 ax = abs(x);
  const s32 ay = abs(y);

  if (x > 0) {
    if (y > 0) {
      if (ax < ay)
        return (1024 - ((ax * 512) / ay));
      else
        return ((ay * 512) / ax);
    } else {
      if (ay < ax)
        return (4096 - ((ay * 512) / ax));
      else
        return (((ax * 512) / ay) + 3072);
    }
  }

  if (y > 0) {
    if (ax < ay)
      return (((ax * 512) / ay) + 1024);
    else
      return (2048 - ((ay * 512) / ax));
  }

  if (ay < ax)
    return (((ay * 512) / ax) + 2048);
  else
    return (3072 - ((ax * 512) / ay));
}

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

x16 VecToYaw(const x32vec3_t *vec) {
  const s32 ix = vec->x >> FIXSHIFT;
  const s32 iy = vec->y >> FIXSHIFT;
  if (ix == 0 && iy == 0)
    return 0;
  return qatan2(iy, ix);
}

MATRIX *RotMatrixZY(SVECTOR *r, MATRIX *m) {
  short s[3], c[3];
  MATRIX tm[2];

  s[1] = isin(r->vy); s[2] = isin(r->vz);
  c[1] = icos(r->vy); c[2] = icos(r->vz);

  // mZ
  tm[1].m[0][0] = c[2];  tm[1].m[0][1] = -s[2]; tm[1].m[0][2] = 0;
  tm[1].m[1][0] = s[2];  tm[1].m[1][1] = c[2];  tm[1].m[1][2] = 0;
  tm[1].m[2][0] = 0;     tm[1].m[2][1] = 0;     tm[1].m[2][2] = ONE;

  // mY
  tm[0].m[0][0] = c[1];  tm[0].m[0][1] = 0;     tm[0].m[0][2] = s[1];
  tm[0].m[1][0] = 0;     tm[0].m[1][1] = ONE;   tm[0].m[1][2] = 0;
  tm[0].m[2][0] = -s[1]; tm[0].m[2][1] = 0;     tm[0].m[2][2] = c[1];

  MulMatrix0(&tm[1], &tm[0], m);

  return m;
}
