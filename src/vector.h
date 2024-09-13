#pragma once

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
