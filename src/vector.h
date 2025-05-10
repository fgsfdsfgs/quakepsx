#pragma once

#include <psxgte.h>
#include "types.h"
#include "fixed.h"

#pragma pack(push, 1)

#define DECLARE_VEC4_T_WORD(ctype, wtype) \
typedef union ctype ## vec4_u { \
  struct { ctype x, y, z, w; }; \
  struct { ctype u, v, p, q; }; \
  struct { ctype r, g, b, a; }; \
  ctype d[4]; \
  wtype word; \
} ctype ## vec4_t

#define DECLARE_VEC3_T(ctype) \
typedef union ctype ## vec3_u { \
  struct { ctype x, y, z; }; \
  struct { ctype u, v, p; }; \
  struct { ctype r, g, b; }; \
  ctype d[3]; \
} ctype ## vec3_t

#define DECLARE_VEC2_T(ctype) \
typedef union ctype ## vec2_u { \
  struct { ctype x, y; }; \
  struct { ctype u, v; }; \
  ctype d[2]; \
} ctype ## vec2_t

#define DECLARE_VEC2_T_WORD(ctype, wtype) \
typedef union ctype ## vec2_u { \
  struct { ctype x, y; }; \
  struct { ctype u, v; }; \
  ctype d[2]; \
  wtype word; \
} ctype ## vec2_t

#pragma pack(pop)

DECLARE_VEC4_T_WORD(u8, u32);

DECLARE_VEC3_T(x32);
DECLARE_VEC3_T(s32);
DECLARE_VEC3_T(x16);
DECLARE_VEC3_T(s16);
DECLARE_VEC3_T(u8);

DECLARE_VEC2_T(x32);
DECLARE_VEC2_T(s32);
DECLARE_VEC2_T_WORD(x16, u32);
DECLARE_VEC2_T_WORD(s16, u32);
DECLARE_VEC2_T_WORD(u8, u16);

extern x32vec3_t x32vec3_origin;
extern x16vec3_t x16vec3_origin;

#define XVecAdd(a, b, c) \
  { (c)->x = (a)->x + (b)->x; (c)->y = (a)->y + (b)->y; (c)->z = (a)->z + (b)->z; }

#define XVecAddInt(a, bx, by, bz, c) \
  { (c)->x = (a)->x + TO_FIX32(bx); (c)->y = (a)->y + TO_FIX32(by); (c)->z = (a)->z + TO_FIX32(bz); }

#define XVecSub(a, b, c) \
  { (c)->x = (a)->x - (b)->x; (c)->y = (a)->y - (b)->y; (c)->z = (a)->z - (b)->z; }

#define XVecDotSS(a, b) \
  ((XMUL16((x32)((a)->x), (x32)((b)->x)) + XMUL16((x32)((a)->y), (x32)((b)->y)) + XMUL16((x32)((a)->z), (x32)((b)->z))))

#define XVecScaleS(a, b, c) \
  { (c)->x = XMUL16((x32)((a)->x), (x32)(b)); (c)->y = XMUL16((x32)((a)->y), (x32)(b)); (c)->z = XMUL16((x32)((a)->z), (x32)(b)); }

#define XVecScaleLS(a, b, c) \
  { (c)->x = xmul32((b), (x32)((a)->x)); (c)->y = xmul32((b), (x32)((a)->y)); (c)->z = xmul32((b), (x32)((a)->z)); }

#define XVecScaleSL(a, b, c) \
  { (c)->x = xmul32((a)->x, (x32)(b)); (c)->y = xmul32((a)->y, (x32)(b)); (c)->z = xmul32((a)->z, (x32)(b)); }

#define XVecScaleInt(a, b, c) \
  { (c)->x = (a)->x * (x32)(b); (c)->y = (a)->y * (x32)(b); (c)->z = (a)->z * (x32)(b); }

#define XVecLengthS(v) \
  (SquareRoot12((v)->x * (v)->x + (v)->y * (v)->y + (v)->z * (v)->z))

#define XVecLengthL(v) \
  (SquareRoot12(XVecLengthSqrL(v)))

#define XVecLengthIntL(v) \
  (SquareRoot0(XVecLengthSqrIntL(v)))

#define XVecZero(v) \
  { (v)->x = 0; (v)->y = 0; (v)->z = 0; }

#define XVecNegate(a, b) \
  { (b)->x = -(a)->x; (b)->y = -(a)->y; (b)->z = -(a)->z; }

#define XVecSet(a, ax, ay, az) \
  { (a)->x = (ax); (a)->y = (ay); (a)->z = (az); }

#define XVecSetInt(a, ax, ay, az) \
  { (a)->x = TO_FIX32(ax); (a)->y = TO_FIX32(ay); (a)->z = TO_FIX32(az); }

#define XVecEq(a, b) \
  ((a)->x == (b)->x && (a)->y == (b)->y && (a)->z == (b)->z)

FORCEINLINE void XVecSignLS(const x32vec3_t *in, x16vec3_t *out) {
  out->x = xsign32(in->x);
  out->y = xsign32(in->y);
  out->z = xsign32(in->z);
}

FORCEINLINE void XVecMulAddLSL(const x32vec3_t *a, const x16 b, const x32vec3_t *c, x32vec3_t *out) {
  out->x = a->x + xmul32(b, c->x);
  out->y = a->y + xmul32(b, c->y);
  out->z = a->z + xmul32(b, c->z);
}

FORCEINLINE void XVecMulAddS(const x16vec3_t *a, const x16 b, const x16vec3_t *c, x16vec3_t *out) {
  out->x = a->x + xmul32(b, c->x);
  out->y = a->y + xmul32(b, c->y);
  out->z = a->z + xmul32(b, c->z);
}

FORCEINLINE x32 XVecDotLL(const x32vec3_t *a, const x32vec3_t *b) {
  return (xmul32(a->x, b->x) + xmul32(a->y, b->y) + xmul32(a->z, b->z));
}

FORCEINLINE x32 XVecDotSL(const x16vec3_t *a, const x32vec3_t *b) {
  return (xmul32(a->x, b->x) + xmul32(a->y, b->y) + xmul32(a->z, b->z));
}

extern x32 XVecLengthSqrL(const x32vec3_t *v);
extern s32 XVecLengthSqrIntL(const x32vec3_t *v);
extern x32 XVecNormLS(const x32vec3_t *in, x16vec3_t *out, x32 *out_xysqrlen);
extern void XVecCrossSS(const x16vec3_t *a, const x16vec3_t *b, x16vec3_t *out);
