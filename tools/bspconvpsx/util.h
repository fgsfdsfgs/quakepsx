#pragma once

#include <stdlib.h>
#include <math.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/util.h"

#define MAX_TOKEN 2048

#define XBSP_SCALE 1

void img_palettize(const u8 *src, u8 *dst, const int w, const int h, const u8 *pal, const int palsiz);
void img_unpalettize(const u8 *src, u8 *dst, const int w, const int h, const u8 *pal);
int img_quantize(const u8 *src, u8 *dst, const int w, const int h, u8 *outpal);

const char *com_parse(const char *data, char *com_token);

int resmap_parse(const char *fname, char *list, const int max_num, const int entry_len, const int name_len);

static inline x32 f32_to_x32(const f32 x) {
  return (x32)(x * (f32)FIXSCALE);
}

static inline x16 f32_to_x16deg(const f32 x) {
  return (x16)(x * (f32)FIXSCALE / 360.f);
}

static inline s16vec2_t qvec2_to_s16vec2(const qvec2_t v) {
  return (s16vec2_t){ (s16)v[0], (s16)v[1] };
}

static inline u8vec2_t qvec2_to_u8vec2(const qvec2_t v) {
  return (u8vec2_t){ (u8)v[0], (u8)v[1] };
}

static inline s16vec3_t qvec3_to_s16vec3(const qvec3_t v) {
  return (s16vec3_t){ (s16)round(v[0]) * XBSP_SCALE, (s16)round(v[1]) * XBSP_SCALE, (s16)round(v[2]) * XBSP_SCALE };
}

static inline x16vec3_t qvec3_to_x16vec3(const qvec3_t v) {
  const x16 x = f32_to_x32(v[0]);
  const x16 y = f32_to_x32(v[1]);
  const x16 z = f32_to_x32(v[2]);
  return (x16vec3_t){ x, y, z };
}

static inline x16vec3_t qvec3_to_x16vec3_angles(const qvec3_t v) {
  const x16 x = f32_to_x16deg(v[0]);
  const x16 y = f32_to_x16deg(v[1]);
  const x16 z = f32_to_x16deg(v[2]);
  return (x16vec3_t){ x, y, z };
}

static inline x32vec3_t qvec3_to_x32vec3(const qvec3_t v) {
  const x32 x = f32_to_x32(v[0]);
  const x32 y = f32_to_x32(v[1]);
  const x32 z = f32_to_x32(v[2]);
  return (x32vec3_t){ x, y, z };
}

static inline f32 qdot2(const qvec2_t a, const qvec2_t b) {
  return a[0] * b[0] + a[1] * b[1];
}

static inline f32 qdot3(const qvec3_t a, const qvec3_t b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline f32 *qvec3_sub(qvec3_t a, qvec3_t b, qvec3_t c) {
  c[0] = a[0] - b[0];
  c[1] = a[1] - b[1];
  c[2] = a[2] - b[2];
  return c;
}

static inline f32 *qvec3_add(qvec3_t a, qvec3_t b, qvec3_t c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
  c[2] = a[2] + b[2];
  return c;
}

static inline f32 *qvec2_sub(qvec2_t a, qvec2_t b, qvec2_t c) {
  c[0] = a[0] - b[0];
  c[1] = a[1] - b[1];
  return c;
}

static inline f32 *qvec2_add(qvec2_t a, qvec2_t b, qvec2_t c) {
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
  return c;
}


static inline f32 qvec3_norm(qvec3_t a) {
  // quake uses sqrt here instead of sqrtf, wonder why
  const f32 len = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  if (len) {
    const f32 s = 1.f / len;
    a[0] *= s;
    a[1] *= s;
    a[2] *= s;
  }
  return len;
}

static inline f32 qvec2_norm(qvec2_t a) {
  const f32 len = sqrt(a[0] * a[0] + a[1] * a[1]);
  if (len) {
    const f32 s = 1.f / len;
    a[0] *= s;
    a[1] *= s;
  }
  return len;
}

static inline f32 *qvec3_scale(qvec3_t src, const f32 s, qvec3_t dst) {
  dst[0] = src[0] * s;
  dst[1] = src[1] * s;
  dst[2] = src[2] * s;
  return dst;
}

static inline f32 *qvec2_scale(qvec2_t src, const f32 s, qvec2_t dst) {
  dst[0] = src[0] * s;
  dst[1] = src[1] * s;
  return dst;
}
static inline f32 *qvec3_copy(qvec3_t src, qvec3_t dst) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  return dst;
}

static inline f32 *qvec2_copy(qvec2_t src, qvec2_t dst) {
  dst[0] = src[0];
  dst[1] = src[1];
  return dst;
}

static inline f32 ffract(const float x) {
  return x - floorf(x);
}

static inline f32 fsfract(const float x) {
  return x - (long)(x);
}

