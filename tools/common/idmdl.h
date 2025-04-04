#pragma once

#include "psxtypes.h"
#include "idbsp.h"

#define ALIAS_VERSION 6
#define ALIAS_ONSEAM  0x0020

#define MAX_SKINS     32
#define MAX_FRAMENAME 16

#pragma pack(push, 1)

typedef struct {
  u8 v[3];
  u8 lightnormalindex;
} qtrivertx_t;

typedef struct {
  s32 onseam;
  s32 s;
  s32 t;
} qaliastexcoord_t;

typedef struct {
  s32 front;
  s32 vertex[3];
} qaliastri_t;

typedef struct {
  s32 type;
  qtrivertx_t min;
  qtrivertx_t max;
  char name[MAX_FRAMENAME];
  qtrivertx_t *verts;
} qaliasframe_t;

typedef struct {
  s32 group;
  u8 *data;
} qaliasskin_t;

typedef struct {
  s32 ident;
  s32 version;
  qvec3_t scale;
  qvec3_t translate;
  f32 boundingradius;
  qvec3_t eyeposition;
  s32 numskins;
  s32 skinwidth;
  s32 skinheight;
  s32 numverts;
  s32 numtris;
  s32 numframes;
  s32 synctype;
  s32 flags;
  f32 size;
} qaliashdr_t;

#pragma pack(pop)
