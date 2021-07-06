#pragma once

#include "psxtypes.h"
#include "idbsp.h"

#define ALIAS_VERSION 6
#define ALIAS_ONSEAM  0x0020

#define	MAX_SKINS	    32
#define MAX_FRAMENAME 16

#pragma pack(push, 1)

typedef struct {
  u8 v[3];
  u8 lightnormalindex;
} qtrivertx_t;

typedef struct {
  s32 firstpose;
  s32 numposes;
  f32 interval;
  qtrivertx_t bboxmin;
  qtrivertx_t bboxmax;
  int frame;
  char name[MAX_FRAMENAME];
} qaliasframedesc_t;

typedef struct {
  s32 ident;
  s32 version;
  qvec3_t scale;
  qvec3_t scale_origin;
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
