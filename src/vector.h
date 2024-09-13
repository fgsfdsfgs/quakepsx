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

extern const x32vec3_t x32vec3_origin;
extern const x16vec3_t x16vec3_origin;

#define XVecAdd(a, b, c) \
  { (c)->x = (a)->x + (b)->x; (c)->y = (a)->y + (b)->y; (c)->z = (a)->z + (b)->z; }

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

#define XVecLengthS(v) \
  (SquareRoot12((v)->x * (v)->x + (v)->y * (v)->y + (v)->z * (v)->z))

#define XVecLengthL(v) \
  (SquareRoot12(XVecLengthSqrL(v)))

FORCEINLINE void XVecSignLS(const x32vec3_t *in, x16vec3_t *out) {
  out->x = xsign32(in->x);
  out->y = xsign32(in->y);
  out->z = xsign32(in->z);
}

extern x32 XVecLengthSqrL(const x32vec3_t *v);
extern x32 XVecNormLS(const x32vec3_t *in, x16vec3_t *out, x32 *out_xysqrlen);

extern x32 XVecDotLL(const x32vec3_t *a, const x32vec3_t *b);
extern x32 XVecDotSL(const x16vec3_t *a, const x32vec3_t *b);
extern void XVecCrossSS(const x16vec3_t *a, const x16vec3_t *b, x16vec3_t *out);
