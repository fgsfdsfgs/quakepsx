#pragma once

#include <psxgte.h>

#define PSX_SCRATCH ((void *)0x1F800000)

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef true
#define true TRUE
#endif

#ifndef false
#define false FALSE
#endif

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef   signed char  s8;
typedef   signed short s16;
typedef   signed int   s32;
typedef   signed short x16; // 1.03.12
typedef   signed int   x32; // 1.19.12
typedef unsigned char  qboolean;

#define FIXSHIFT 12
#define FIXSCALE (1 << 12)
#ifndef ONE
#define ONE FIXSCALE
#endif
#define ONEDEGREE (FIXSCALE / 360)
#define HALF (ONE >> 1)

#define TO_FIX32(x) ((x32)(x) << FIXSHIFT)
#define TO_DEG16(x) (((x32)(x) << FIXSHIFT) / 360)

// these will almost always overflow with 32-bit stuff
#define XMUL16(x, y) (((x32)(x) * (x32)(y)) >> FIXSHIFT)
#define XDIV16(x, y) (((x32)(x) * FIXSCALE) / (x32)(y))

extern x32 xmul32(const x32 x, const x32 y);
extern x32 xdiv32(const x32 x, const x32 y);

#define FIX(x) (x << FIXSHIFT)

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

DECLARE_VEC2_T(x32);
DECLARE_VEC2_T(s32);
DECLARE_VEC2_T(x16);
DECLARE_VEC2_T(s16);
DECLARE_VEC2_T(u8);

#pragma pack(pop)

typedef struct link_s {
  struct link_s *prev, *next;
} link_t;

#define	STRUCT_FROM_LINK(l, t, m) ((t *)((u8 *)l - (int)&(((t *)0)->m)))
