#pragma once

#include "psxtypes.h"
#include "psxmdl.h"

#define PSXBSPVERSION 0x1D585350

#define MAX_XMAP_HULLS      4
#define MAX_XMAP_VERTS      65535
#define MAX_XMAP_PLANES     32767
#define MAX_XMAP_TEXTURES   255
#define MAX_XMAP_NODES      32767
#define MAX_XMAP_CLIPNODES  32767
#define MAX_XMAP_LEAFS      8192
#define MAX_XMAP_FACES      32767
#define MAX_XMAP_MODELS     256
#define MAX_XMAP_MARKSURF   65535
#define MAX_XMAP_ENTITIES   512
#define MAX_XMAP_ENTMODELS  128
#define MAX_XMAP_VISIBILITY 0x40000
#define MAX_XMAP_LIGHTVALS  2

#define MAX_ENTITIES        1024
#define MAX_SOUNDS          255
#define MAX_LIGHTSTYLES     64

#define MAX_TEX_WIDTH       64
#define MAX_TEX_HEIGHT      64

#define VRAM_XSTART         320
#define VRAM_NUM_PAGES      2
#define VRAM_PAGE_WIDTH     (1024 - 64 - VRAM_XSTART)
#define VRAM_PAGE_HEIGHT    256
#define VRAM_TEXPAGE_WIDTH  64
#define VRAM_TEXPAGE_HEIGHT 256
#define VRAM_TOTAL_WIDTH    (VRAM_PAGE_WIDTH)
#define VRAM_TOTAL_HEIGHT   (VRAM_PAGE_HEIGHT * VRAM_NUM_PAGES)

#define SPURAM_BASE         0x1100
#define SPURAM_SIZE         (0x80000 - SPURAM_BASE)

#define NUM_CLUT_COLORS     256

#pragma pack(push, 1)

/* XBSP STRUCTURE
 * xbsphdr_t  ver="PSX\x1D"
 * xbsplump_t XLMP_CLUTDATA
 * xbsplump_t XLMP_TEXDATA
 * xbsplump_t XLMP_SNDDATA
 * xbsplump_t XLMP_MDLDATA
 * xbsplump_t XLMP_VERTS
 * xbsplump_t XLMP_PLANES
 * xbsplump_t XLMP_TEXINFO
 * xbsplump_t XLMP_FACES
 * xbsplump_t XLMP_MARKSURF
 * xbsplump_t XLMP_VISILIST
 * xbsplump_t XLMP_LEAFS
 * xbsplump_t XLMP_NODES
 * xbsplump_t XLMP_CLIPNODES
 * xbsplump_t XLMP_MODELS
 * xbsplump_t XLMP_ENTITIES
 */

enum xbsplump_e {
  XLMP_INVALID = -1,

  XLMP_CLUTDATA,
  XLMP_TEXDATA,
  XLMP_SNDDATA,
  XLMP_MDLDATA,

  XLMP_VERTS,
  XLMP_PLANES,
  XLMP_TEXINFO,
  XLMP_FACES,
  XLMP_MARKSURF,
  XLMP_VISILIST,
  XLMP_LEAFS,
  XLMP_NODES,
  XLMP_CLIPNODES,
  XLMP_MODELS,
  XLMP_ENTITIES,

  XLMP_COUNT
};

typedef struct {
  s16vec3_t pos; // positions are always integer
  u8vec2_t tex;  // 0-255 or 0-size
  u8 col[MAX_XMAP_LIGHTVALS]; // light values for every used lightstyle
} xvert_t;

typedef struct {
  s32 type;
  s32 size;
  u8 data[];
} xlump_t;

typedef struct {
  u32 ver;
} xbsphdr_t;

typedef struct {
  x16vec3_t normal;
  x32 dist;
  s32 type;
} xplane_t;

typedef struct {
  u16 planenum;
  s16 children[2]; // negative numbers are -(leafs+1), not nodes
  s16vec3_t mins;  // for sphere culling
  s16vec3_t maxs;
  u16 firstface;
  u16 numfaces;    // counting both sides
} xnode_t;

typedef struct {
  s16 planenum;
  s16 children[2]; // negative numbers are contents
} xclipnode_t;

typedef struct {
  u8vec2_t uv;
  s16vec2_t size;
  u16 tpage;
  u16 flags;
} xtexinfo_t;

#define XTEX_SPECIAL   1
#define XTEX_LIQUID    2
#define XTEX_SKY       4
#define XTEX_INVISIBLE 8
#define XTEX_ANIMATED  16
#define XTEX_NULL      0x8000

typedef struct {
  s16 planenum;
  s16 side;
  s32 firstvert;
  s16 numverts;
  s16 texinfo;
  u8 styles[MAX_XMAP_LIGHTVALS];
} xface_t;

typedef struct {
  s16 contents;
  s32 visofs;     // -1 = no visibility info
  s16vec3_t mins; // for frustum culling
  s16vec3_t maxs;
  u16 firstmarksurface;
  u16 nummarksurfaces;
  u8 lightmap[MAX_XMAP_LIGHTVALS];
  u8 styles[MAX_XMAP_LIGHTVALS];
} xleaf_t;

typedef struct {
  s16vec3_t mins;
  s16vec3_t maxs;
  s16vec3_t origin;
  s16 headnode[MAX_XMAP_HULLS];
  s16 visleafs;
  u16 firstface;
  u16 numfaces;
} xmodel_t;

typedef struct {
  u8 classname;
  u16 spawnflags;
  s16 model; // negative = brush models, positive = alias models
  x32vec3_t origin;
  x16vec3_t angles;
} xmapent_t;

typedef struct {
  s16 soundid;
  u16 frames;
  u32 spuaddr;
} xmapsnd_t;

typedef struct {
  u32 nummdls;
  xaliashdr_t mdls[];
} xmdllump_t;

typedef struct {
  u32 numsfx;
  xmapsnd_t sfx[];
} xsndlump_t;

typedef struct {
  u16 tpage;
  u8vec2_t uv;
  u8vec2_t size;
} xpic_t;

#pragma pack(pop)
