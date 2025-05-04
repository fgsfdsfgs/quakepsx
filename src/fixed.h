#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef int16_t x16; // 1.03.12 fixed point
typedef int32_t x32; // 1.19.12 fixed point

#define FIXSHIFT 12
#define FIXSCALE (1 << 12)

#ifndef ONE
#define ONE FIXSCALE
#endif

#define ONEDEGREE (FIXSCALE / 360)

#define HALF (ONE >> 1)

#define TO_FIX32(x) ((x32)(x) << FIXSHIFT)
#define TO_DEG16(x) (((x32)(x) << FIXSHIFT) / 360)

// these will almost always overflow with 32-bit stuff, so only use them for x16 * x16 or x16 * x32
#define XMUL16(x, y) (((x32)(x) * (x32)(y)) >> FIXSHIFT)
#define XDIV16(x, y) (((x32)(x) << FIXSHIFT) / (x32)(y))

// only use for constants
#define XRECIP(x) (x32)(((double)ONE) / ((double)(x)))
#define FTOX(x) (x32)(((double)ONE) * ((double)(x)))

// NOTE: these compile into trash without at least -O2

#pragma GCC push_options
#pragma GCC optimize("-O3")

FORCEINLINE x32 xmul32(const x32 x, const x32 y) {
  return ((s64)x * y) >> FIXSHIFT;
}

FORCEINLINE x32 xdiv32(const x32 x, const x32 y) {
  return ((x << FIXSHIFT) / y);
}

FORCEINLINE x32 xsign32(const x32 x) {
  return (x > 0) - (x < 0);
}

FORCEINLINE x32 xrand32(void) {
  return rand() & (ONE - 1);
}

#pragma GCC pop_options
