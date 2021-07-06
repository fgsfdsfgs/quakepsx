#pragma once

#include "common.h"

#define PSXBSPVERSION 0x1D585350

#define MAX_MAP_HULLS      4
#define MAX_MAP_VERTS      65535
#define MAX_MAP_PLANES     32767
#define MAX_MAP_TEXTURES   255
#define MAX_MAP_NODES      32767
#define MAX_MAP_CLIPNODES  32767
#define MAX_MAP_LEAFS      8192
#define MAX_MAP_FACES      32767
#define MAX_MAP_MODELS     256
#define MAX_MAP_MARKSURF   65535
#define MAX_MAP_VISIBILITY 0x40000
#define MAX_MAP_LIGHTVALS  2 // different light values on a single surface

#pragma pack(push, 1)

/* XBSP STRUCTURE
 * xbsphdr_t  ver="PSX\x1D"
 * xbsplump_t LUMP_CLUTDATA
 * xbsplump_t LUMP_TEXDATA
 * xbsplump_t LUMP_SNDDATA
 * xbsplump_t LUMP_MDLDATA
 * xbsplump_t LUMP_VERTS
 * xbsplump_t LUMP_PLANES
 * xbsplump_t LUMP_TEXINFO
 * xbsplump_t LUMP_FACES
 * xbsplump_t LUMP_MARKSURF
 * xbsplump_t LUMP_VISILIST
 * xbsplump_t LUMP_LEAFS
 * xbsplump_t LUMP_NODES
 * xbsplump_t LUMP_CLIPNODES
 * xbsplump_t LUMP_MODELS
 * xbsplump_t LUMP_ENTITIES
 */

enum xbsplump_e {
  LUMP_INVALID = -1,

  LUMP_CLUTDATA,
  LUMP_TEXDATA,
  LUMP_SNDDATA,
  LUMP_MDLDATA,

  LUMP_VERTS,
  LUMP_PLANES,
  LUMP_TEXINFO,
  LUMP_FACES,
  LUMP_MARKSURF,
  LUMP_VISILIST,
  LUMP_LEAFS,
  LUMP_NODES,
  LUMP_CLIPNODES,
  LUMP_MODELS,
  LUMP_ENTITIES,

  LUMP_COUNT
};

typedef struct {
  s16vec3_t pos; // positions are always integer
  u8vec2_t tex;  // 0-255 or 0-size
  u8 col[MAX_MAP_LIGHTVALS]; // light values for every lightstyle
} xbspvert_t;

typedef struct {
  s32 type;
  s32 size;
  u8 data[];
} xbsplump_t;

typedef struct {
  u32 ver;
} xbsphdr_t;

typedef struct {
  x16vec3_t normal;
  x32 dist;
  s32 type;
} xbspplane_t;

typedef struct {
  u16 planenum;
  s16 children[2]; // negative numbers are -(leafs+1), not nodes
  s16vec3_t mins;  // for sphere culling
  s16vec3_t maxs;
  u16 firstface;
  u16 numfaces;    // counting both sides
} xbspnode_t;

typedef struct {
  s16 planenum;
  s16 children[2]; // negative numbers are contents
} xbspclipnode_t;

typedef struct {
  u8vec2_t uv;
  s16vec2_t size;
  u16 tpage;
  u16 flags;
} xbsptexinfo_t;

typedef struct {
  s16 planenum;
  s16 side;
  s32 firstvert;
  s16 numverts;
  s16 texinfo;
  u8 styles[MAX_MAP_LIGHTVALS];
} xbspface_t;

typedef struct {
  s16 contents;
  s32 visofs;     // -1 = no visibility info
  s16vec3_t mins; // for frustum culling
  s16vec3_t maxs;
  u16 firstmarksurface;
  u16 nummarksurfaces;
} xbspleaf_t;

typedef struct {
  s16vec3_t mins;
  s16vec3_t maxs;
  s16vec3_t origin;
  s16 headnode[MAX_MAP_HULLS];
  s16 visleafs;
  u16 firstface;
  u16 numfaces;
} xbspmodel_t;

typedef struct {
  u16 ofs;
  s32 val;
} xbspentfield_t;

typedef struct {
  u8 classname;
  u8 numfields;
  xbspentfield_t fields[];
} xbspent_t;

typedef struct {
  u8 soundid;
  u32 spuaddr;
  u32 size;
  u8 data[];
} xbspsnd_t;

#pragma pack(pop)
