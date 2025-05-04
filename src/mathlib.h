#pragma once

#include "types.h"
#include "fixed.h"
#include "vector.h"
#include "model.h"

enum angleidx_e { PITCH, YAW, ROLL };

x16 qatan2(s32 y, s32 x);

void AngleVectors(const x16vec3_t *angles, x16vec3_t *forward, x16vec3_t *right, x16vec3_t *up);
int BoxOnPlaneSide(const x32vec3_t *emins, const x32vec3_t *emaxs, const mplane_t *p);

x16 VecToYaw(const x32vec3_t *vec);

MATRIX *RotMatrixZY(SVECTOR *r, MATRIX *m);

static inline x16 VecDeltaToYaw(const x32vec3_t *va, const x32vec3_t *vb) {
  x32vec3_t delta;
  XVecSub(va, vb, &delta);
  return VecToYaw(&delta);
}
