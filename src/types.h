#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <psxgte.h>

#include "fixed.h"

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

typedef  uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef   int8_t s8;
typedef  int16_t s16;
typedef  int32_t s32;
typedef  int64_t s64;
typedef       u8 qboolean;

#include "vector.h"

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

typedef struct link_s {
  struct link_s *prev, *next;
} link_t;

#define STRUCT_FROM_LINK(l, t, m) ((t *)((u8 *)l - (int)&(((t *)0)->m)))
