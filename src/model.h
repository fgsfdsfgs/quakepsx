#pragma once

#include "common.h"
#include "bspfile.h"

// 0-2 are axial planes
#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2
// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

#define SIDE_FRONT 0
#define SIDE_BACK  1
#define SIDE_ON    2

#define CONTENTS_EMPTY         -1
#define CONTENTS_SOLID         -2
#define CONTENTS_WATER         -3
#define CONTENTS_SLIME         -4
#define CONTENTS_LAVA          -5
#define CONTENTS_SKY           -6
#define CONTENTS_ORIGIN        -7 // removed at csg time
#define CONTENTS_CLIP          -8 // changed to contents_solid
#define CONTENTS_CURRENT_0     -9
#define CONTENTS_CURRENT_90   -10
#define CONTENTS_CURRENT_180  -11
#define CONTENTS_CURRENT_270  -12
#define CONTENTS_CURRENT_UP   -13
#define CONTENTS_CURRENT_DOWN -14

#define SURF_PLANEBACK      2
#define SURF_DRAWSKY        4
#define SURF_DRAWSPRITE     8
#define SURF_DRAWTURB       0x10
#define SURF_DRAWTILED      0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER     0x80

// in memory representations of bspfile.h structs

/* BSP MODELS */

typedef xbspvert_t mvert_t;

typedef xbspclipnode_t mclipnode_t;

typedef struct {
  x16vec3_t normal;
  x32 dist;
  u8 type;
  u8 signbits; // signx + signy<<1 + signz<<1
} mplane_t;

typedef struct msurface_s msurface_t;

typedef struct mtexture_s {
  struct mtexture_s *anim_next;
  struct msurface_s *texchain;
  s16 width;
  s16 height;
  u16 vram_page;
  u8 vram_u;
  u8 vram_v;
  s8 anim_total;
  s8 anim_min;
  s8 anim_max;
  u8 flags;
} mtexture_t;

struct msurface_s {
  mplane_t *plane;
  mtexture_t *texture;
  struct msurface_s *texchain;
  u8 styles[MAX_MAP_LIGHTVALS];
  u16 firstvert;
  u16 numverts;
  u8 visframe;
  u8 flags;
};

typedef struct mnode_s {
  s8 contents;
  u8 visframe;
  x32vec3_t mins;
  x32vec3_t maxs;
  struct mnode_s *parent;
  mplane_t *plane;
  struct mnode_s *children[2];
  u16 firstsurf;
  u16 numsurf;
} mnode_t;

typedef struct mleaf_s {
  s8 contents;
  u8 visframe;
  x32vec3_t mins;
  x32vec3_t maxs;
  struct mnode_s *parent;
  u8 *compressed_vis;
  msurface_t **firstmarksurf;
  u16 nummarksurf;
  s32 key;
} mleaf_t;

typedef struct {
  xbspclipnode_t *clipnodes;
  mplane_t *planes;
  u16 firstclipnode;
  u16 lastclipnode;
  x32vec3_t mins;
  x32vec3_t maxs;
} hull_t;

/* WHOLE MODEL */

typedef enum { mod_brush, mod_sprite, mod_alias } modtype_t;

#define EF_ROCKET  1   // leave a trail
#define EF_GRENADE 2   // leave a trail
#define EF_GIB     4   // leave a trail
#define EF_ROTATE  8   // rotate (bonus items)
#define EF_TRACER  16  // green split trail
#define EF_ZOMGIB  32  // small blood trail
#define EF_TRACER2 64  // orange split trail + rotate
#define EF_TRACER3 128 // purple trail

typedef struct model_s {
  s16 id; // <0 == brush models
  s16 type;

  int flags;

  x32vec3_t mins, maxs;
  x32 radius;

  int firstmodelsurface;
  int nummodelsurfaces;

  int numsubmodels;
  xbspmodel_t *submodels;

  int numplanes;
  mplane_t *planes;

  int numleafs; // number of visible leafs, not counting 0
  mleaf_t *leafs;

  int numverts;
  mvert_t *verts;

  int numnodes;
  mnode_t *nodes;

  int numsurfaces;
  msurface_t *surfaces;

  int numclipnodes;
  xbspclipnode_t *clipnodes;

  int nummarksurfaces;
  msurface_t **marksurfaces;

  hull_t hulls[MAX_MAP_HULLS];

  int numtextures;
  mtexture_t *textures;

  u8 *visdata;
} model_t;

model_t *Mod_LoadModel(const char *name);
mleaf_t *Mod_PointInLeaf(const x32vec3_t p, model_t *mod);
const u8 *Mod_LeafPVS(const mleaf_t *leaf, const model_t *model);
