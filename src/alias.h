#pragma once

#include "common.h"

/* XMDL STRUCTURE
 * aliashdr_t      hdr;
 * aliastri_t      tris[hdr.numtris];
 * aliastexcoord_t texcoords[hdr.numverts];
 * aliasvert_t     frames[hdr.numframes][hdr.numverts];
 */

#define MAX_XMDL_VERTS  256
#define MAX_XMDL_FRAMES 256
#define MAX_XMDL_TRIS   1024

#pragma pack(push, 1)

typedef struct {
  u8 verts[3];
  u8 fnorm;
} atri_t;

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
  atri_t *tris;
  u8vec3_t *texcoords;
  u8vec3_t *frames;
} aliashdr_t;

#pragma pack(pop)
