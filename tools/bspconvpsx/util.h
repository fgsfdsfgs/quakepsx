#pragma once

#include <stdlib.h>
#include <math.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define PSXRGB(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5) | ((r) >> 3))
#define PSXTPAGE(tp, abr, x, y) ((((x)&0x3FF)>>6) | (((y)>>8)<<4) | (((abr)&0x3)<<5) | (((tp)&0x3)<<7))

void panic(const char *fmt, ...);
void *lmp_read(const char *dir, const char *fname, size_t *outsize);

static inline x32 f32_to_x32(const f32 x) {
  return (x32)(x * (f32)FIXSCALE);
}

static inline s16vec2_t qvec2_to_s16vec2(const qvec2_t v) {
  return (s16vec2_t){ (s16)v[0], (s16)v[1] };
}

static inline u8vec2_t qvec2_to_u8vec2(const qvec2_t v) {
  return (u8vec2_t){ (u8)v[0], (u8)v[1] };
}

static inline s16vec3_t qvec3_to_s16vec3(const qvec3_t v) {
  return (s16vec3_t){ (s16)v[0], (s16)v[1], (s16)v[2] };
}

static inline x16vec3_t qvec3_to_x16vec3(const qvec3_t v) {
  const x16 x = f32_to_x32(v[0]);
  const x16 y = f32_to_x32(v[1]);
  const x16 z = f32_to_x32(v[2]);
  return (x16vec3_t){ x, y, z };
}

static inline x32vec3_t qvec3_to_x32vec3(const qvec3_t v) {
  const x32 x = f32_to_x32(v[0]);
  const x32 y = f32_to_x32(v[1]);
  const x32 z = f32_to_x32(v[2]);
  return (x32vec3_t){ x, y, z };
}

static inline f32 qdot(const qvec3_t a, const qvec3_t b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline f32 ffract(const float x) {
  return x - floorf(x);
}

static inline f32 fsfract(const float x) {
  return x - (long)(x);
}
