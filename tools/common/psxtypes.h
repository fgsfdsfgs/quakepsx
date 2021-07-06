#pragma once

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef   signed char  s8;
typedef   signed short s16;
typedef   signed int   s32;
typedef   signed short x16; // 1.03.12
typedef   signed int   x32; // 1.19.12
typedef          float f32; // not actually used on the psx, but eh

#define FIXSHIFT 12
#define FIXSCALE (1 << 12)
#define ONE FIXSCALE
#define HALF (ONE >> 1)

static inline x32 xmul(const x32 x, const x32 y) {
  return (x * y) >> FIXSHIFT;
}

static inline x32 xdiv(const x32 x, const x32 y) {
  return (x * FIXSCALE) / y;
}

#pragma pack(push, 1)

#define DECLARE_VEC3_T(ctype) \
typedef union ctype ## vec3_u { \
  struct { ctype x, y, z; }; \
  struct { ctype u, v, w; }; \
  struct { ctype r, g, b; }; \
  ctype d[3]; \
} ctype ## vec3_t

#define DECLARE_VEC2_T(ctype) \
typedef union ctype ## vec2_u { \
  struct { ctype x, y; }; \
  struct { ctype u, v; }; \
  ctype d[2]; \
} ctype ## vec2_t

DECLARE_VEC3_T(x32);
DECLARE_VEC3_T(s32);
DECLARE_VEC3_T(x16);
DECLARE_VEC3_T(s16);
DECLARE_VEC3_T(u8);
DECLARE_VEC3_T(s8);

DECLARE_VEC2_T(x32);
DECLARE_VEC2_T(s32);
DECLARE_VEC2_T(x16);
DECLARE_VEC2_T(s16);
DECLARE_VEC2_T(u8);

#pragma pack(pop)
