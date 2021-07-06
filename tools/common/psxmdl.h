#pragma once

#include "psxtypes.h"

#pragma pack(push, 1)

/* XMDL STRUCTURE
 * xmdlhdr_t   hdr;
 * xmdlbvert_t bverts[hdr.numverts];
 * xmdldvert_t dverts[hdr.numframes-1][hdr.numverts];
 */

typedef struct {
  s16vec3_t pos;
  u8vec2_t tex;
  u8 norm; // index in anorms table
} xmdlbvert_t;

// these store offsets from bverts for each frame after 0
typedef s8vec3_t xmdldvert_t;

typedef struct {
  u32 ver;
  u16 id; // index in modelmap
  u16 numverts;
  u16 numtris;
  u16 numframes;
  u8vec2_t skin_uv;
  u16 skin_tpage;
  s16vec3_t mins;
  s16vec3_t maxs;
  x32 radius;
  // these are replaced by pointers in the in-memory struct
  u32 bvertofs;
  u32 dvertofs;
  u32 frameofs[]; // [numframes]
} xmdlhdr_t;

#pragma pack(pop)
