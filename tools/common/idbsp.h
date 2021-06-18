#pragma once

#include "psxtypes.h"

#define QBSPVERSION 29

#define MAX_MAP_HULLS 4
#define MAX_LIGHTMAPS 4
#define MAX_TEXNAME   16
#define NUM_AMBIENTS  4  // automatic ambient sounds
#define NUM_MIPLEVELS 4

#define	MAX_MAP_MODELS       256
#define MAX_MAP_PLANES       32767
#define MAX_MAP_NODES        32767  // because negative shorts are contents
#define MAX_MAP_CLIPNODES    32767  //
#define MAX_MAP_LEAFS        8192
#define MAX_MAP_VERTS        65535
#define MAX_MAP_FACES        65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO      4096
#define MAX_MAP_EDGES        256000
#define MAX_MAP_SURFEDGES    512000
#define MAX_MAP_TEXTURES     512
#define MAX_MAP_MIPTEX       0x200000
#define MAX_MAP_LIGHTING     0x100000
#define MAX_MAP_VISIBILITY   0x100000

#pragma pack(push, 1)

typedef f32 qvec2_t[2];
typedef f32 qvec3_t[3];
typedef f32 qvec4_t[4];

typedef struct {
  qvec3_t v;
} qvert_t;

typedef struct {
  s32 ofs;
  s32 len;
} qlump_t;

typedef struct {
  s32 ver;
  qlump_t entities;
  qlump_t planes;
  qlump_t miptex;
  qlump_t vertices;
  qlump_t visilist;
  qlump_t nodes;
  qlump_t texinfo;
  qlump_t faces;
  qlump_t lightmaps;
  qlump_t clipnodes;
  qlump_t leafs;
  qlump_t marksurfaces;
  qlump_t edges;
  qlump_t surfedges;
  qlump_t models;
} qbsphdr_t;

typedef struct {
  s32 nummiptex;
  s32 dataofs[]; // [nummiptex]
} qmiptexlump_t;

typedef struct {
  qvec3_t mins;
  qvec3_t maxs;
  qvec3_t origin;
  s32 headnode[MAX_MAP_HULLS];
  s32 visleafs;
  s32 firstface;
  s32 numfaces;
} qmodel_t;

// 0-2 are axial planes
#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2
// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

typedef struct {
  qvec3_t normal;
  f32 dist;
  s32 type;
} qplane_t;

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

typedef struct {
  s32 planenum;
  s16 children[2]; // negative numbers are -(leafs+1), not nodes
  s16 mins[3];     // for sphere culling
  s16 maxs[3];
  u16 firstface;
  u16 numfaces;    // counting both sides
} qnode_t;

typedef struct {
  s32 planenum;
  s16 children[2]; // negative numbers are contents
} qclipnode_t;

typedef struct {
  s16 planenum;
  s16 side;
  s32 firstedge;
  s16 numedges;
  s16 texinfo;
  u8 styles[MAX_LIGHTMAPS];
  s32 lightofs; // start of [numstyles*surfsize] samples
} qface_t;

typedef struct {
  s32 contents;
  s32 visofs;  // -1 = no visibility info
  s16 mins[3]; // for frustum culling
  s16 maxs[3];
  u16 firstmarksurface;
  u16 nummarksurfaces;
  u8 ambientlevel[NUM_AMBIENTS];
} qleaf_t;

typedef struct {
  u16 v[2]; // vertex numbers
} qedge_t;

typedef struct {
  char name[MAX_TEXNAME];
  u32 width, height;
  u32 offsets[NUM_MIPLEVELS];
} qmiptex_t;

#define TEXF_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision

typedef struct {
  qvec4_t vecs[2]; // [s/t][xyzw]
  s32 miptex;
  s32 flags;
} qtexinfo_t;

#pragma pack(pop)
