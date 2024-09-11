#pragma once

#include <sys/types.h>
#include <libgte.h>
#include <inline_n.h>
#include "types.h"
#include "model.h"

extern const x32vec3_t x32vec3_origin;
extern const x16vec3_t x16vec3_origin;

enum angleidx_e { PITCH, YAW, ROLL };

#define XVecAdd(a, b, c) \
  { (c)->x = (a)->x + (b)->x; (c)->y = (a)->y + (b)->y; (c)->z = (a)->z + (b)->z; }

#define XVecSub(a, b, c) \
  { (c)->x = (a)->x - (b)->x; (c)->y = (a)->y - (b)->y; (c)->z = (a)->z - (b)->z; }

#define XVecDotS(a, b) \
  ((XMUL16((x32)((a)->x), (x32)((b)->x)) + XMUL16((x32)((a)->y), (x32)((b)->y)) + XMUL16((x32)((a)->z), (x32)((b)->z))))

#define XVecScaleS(a, b, c) \
  { (c)->x = XMUL16((x32)((a)->x), (x32)(b)); (c)->y = XMUL16((x32)((a)->y), (x32)(b)); (c)->z = XMUL16((x32)((a)->z), (x32)(b)); }

#define XVecScaleLS(a, b, c) \
  { (c)->x = xmul32((b), (x32)((a)->x)); (c)->y = xmul32((b), (x32)((a)->y)); (c)->z = xmul32((b), (x32)((a)->z)); }

#define XVecScaleSL(a, b, c) \
  { (c)->x = xmul32((a)->x, (x32)(b)); (c)->y = xmul32((a)->y, (x32)(b)); (c)->z = xmul32((a)->z, (x32)(b)); }

#define XVecLengthS(v) \
  (SquareRoot12((v)->x * (v)->x + (v)->y * (v)->y + (v)->z * (v)->z))

#define XVecDotLL(a, b) \
  (XVecDot32x32((a), (b)))

#define XVecDotSL(a, b) \
  (XVecDot16x32((a), (b)))

#define XVecLengthL(v) \
  (SquareRoot12(XVecLengthSqr32(v)))

extern x32 XVecLengthSqr32(const x32vec3_t *v);
extern x32 XVecDot32x32(const x32vec3_t *a, const x32vec3_t *b);
extern x32 XVecDot16x32(const x16vec3_t *a, const x32vec3_t *b);
extern void XVecCross16(const x16vec3_t *a, const x16vec3_t *b, x16vec3_t *out);

void AngleVectors(const x16vec3_t *angles, x16vec3_t *forward, x16vec3_t *right, x16vec3_t *up);
int BoxOnPlaneSide(const x32vec3_t *emins, const x32vec3_t *emaxs, const mplane_t *p);
