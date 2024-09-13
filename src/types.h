#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <psxgte.h>

#define FORCEINLINE __attribute__((always_inline)) inline

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
