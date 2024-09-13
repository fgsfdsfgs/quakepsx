#pragma once

#include <stdint.h>

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
#define XDIV16(x, y) (((x32)(x) * FIXSCALE) / (x32)(y))

__attribute__((always_inline)) inline x32 xmul32(const x32 x, const x32 y) {
  x32 r;
  __asm__ volatile(
    "mult %1, %2;"
    "mflo %0;"
    "mfhi $t0;"
    "srl  %0, 12;"
    "and  $t0, 0x0FFF;"
    "sll  $t0, 20;"
    "or   %0, $t0;"
    : "=r"(r)
    : "r"(x), "r"(y)
    : "$t0"
  );
  return r;
}

__attribute__((always_inline)) inline x32 xdiv32(const x32 x, const x32 y) {
  x32 r;
  __asm__ volatile(
    "sll  %1, 12;"
    "div  %1, %2;"
    "mflo %0;"
    : "=r"(r)
    : "r"(x), "r"(y)
    :
  );
  return r;
}
