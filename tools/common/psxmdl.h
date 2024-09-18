#pragma once

#include "psxtypes.h"

#pragma pack(push, 1)

/* XMDL STRUCTURE
 * xaliashdr_t      hdr;
 * xaliastri_t      tris[hdr.numtris];
 * xaliastexcoord_t texcoords[hdr.numverts];
 * xaliasvert_t     frames[hdr.numframes][hdr.numverts];
 */

#define MAX_XMDL_VERTS  256
#define MAX_XMDL_FRAMES 256
#define MAX_XMDL_TRIS   1024

// real coord = verts[i] * scale + offset
typedef u8vec3_t xaliasvert_t;

// u, v = normal uv
// w = u for onseam tris
typedef u8vec3_t xaliastexcoord_t;

typedef struct {
  u8 verts[3];
  u8 fnorm; // top bit: backface, bottom 7 bits: normal index
} xaliastri_t;

typedef struct {
  s16 type;
  s16 id;
  u8 numframes;
  u8 numverts;
  u16 numtris;
  u16 tpage;
  x16vec3_t scale;
  s16vec3_t offset;
  x32vec3_t mins;
  x32vec3_t maxs;
  u32 trisofs;
  u32 texcoordsofs; // count: 2 * numverts
  u32 framesofs; // real v = verts[v] * scale + offset
} xaliashdr_t;

#pragma pack(pop)
