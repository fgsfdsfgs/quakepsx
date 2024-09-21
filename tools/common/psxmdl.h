#pragma once

#include "psxtypes.h"

#pragma pack(push, 1)

/* XMDL STRUCTURE
 * xaliashdr_t      hdr;
 * xaliastri_t      tris[hdr.numtris];
 * xaliasvert_t     frames[hdr.numframes][hdr.numverts];
 */

#define MAX_XMDL_VERTS  256
#define MAX_XMDL_FRAMES 256
#define MAX_XMDL_TRIS   1024

// real coord = verts[i] * scale + offset
typedef u8vec3_t xaliasvert_t;

typedef u8vec2_t xaliastexcoord_t;

typedef struct {
  u8vec2_t texcoords[3];
  u8 verts[3];
  u8 normal;
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
  u32 framesofs; // real v = verts[v] * scale + offset
} xaliashdr_t;

#pragma pack(pop)
