#pragma once

#include "types.h"
#include "vector.h"

/* XMDL STRUCTURE
 * aliashdr_t      hdr;
 * aliastri_t      tris[hdr.numtris];
 * aliasvert_t     frames[hdr.numframes][hdr.numverts];
 */

#define MAX_XMDL_VERTS  512
#define MAX_XMDL_FRAMES 256
#define MAX_XMDL_TRIS   1024
#define MAX_XMDL_SKINS  3

#pragma pack(push, 1)

typedef struct {
  u8vec2_t uvs[3];
  u16 verts[3];
} atri_t;

typedef struct {
  u16 tpage;
  u8vec2_t base;
} askin_t;

typedef struct {
  u8 type;
  u8 flags;
  s16 id;
  u16 numframes;
  u16 numverts;
  u16 numtris;
  u16 numskins;
  x16vec3_t scale;
  s16vec3_t offset;
  x32vec3_t mins;
  x32vec3_t maxs;
  askin_t skins[MAX_XMDL_SKINS];
  atri_t *tris;
  u8vec3_t *frames;
} aliashdr_t;

#pragma pack(pop)
