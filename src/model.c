#include <stdlib.h>
#include <psxgpu.h>

#include "common.h"
#include "system.h"
#include "bspfile.h"
#include "render.h"
#include "model.h"

static x32 RadiusFromBounds(const x32vec3_t mins, const x32vec3_t maxs) {
  x32vec3_t corner;
  for (int i = 0; i < 3; i++)
    corner.d[i] = abs(mins.d[i]) > abs(maxs.d[i]) ? abs(mins.d[i]) : abs(maxs.d[i]);
  return XVecLengthL(corner);
}

void Mod_SetParent(mnode_t *node, mnode_t *parent) {
  node->parent = parent;
  if (node->contents < 0)
    return;
  Mod_SetParent(node->children[0], node);
  Mod_SetParent(node->children[1], node);
}

static inline void CheckLumpHeader(xbsplump_t *lump, const int etype, const int esize, const int fh) {
  Sys_FileRead(fh, lump, sizeof(*lump));
  if (lump->type != etype)
    Sys_Error("malformed PSB: expected lump %02x, found %02x", etype, lump->type);
  if (lump->size < esize)
    Sys_Error("malformed PSB: expected at least %d bytes, got %d", esize, lump->size);
}

#define INIT_LUMP(lump, etype, esize, fh) \
  xbsplump_t lump; \
  CheckLumpHeader(&lump, etype, esize, fh)

static void Mod_LoadClutData(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_CLUTDATA, VID_NUM_COLORS * 2, fh);
  u16 clut[VID_NUM_COLORS];
  Sys_FileRead(fh, clut, VID_NUM_COLORS * 2);
  R_UploadClut(clut);
}

static void Mod_LoadTextureData(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_TEXDATA, 0, fh);
  const int lines = lump.size / (VRAM_TEX_WIDTH * 2);
  ASSERT(lines <= VRAM_TEX_HEIGHT);
  u8 *data = Mem_Alloc(lump.size);
  Sys_FileRead(fh, data, lump.size);
  R_UploadTexture(data, VRAM_TEX_XSTART, VRAM_TEX_YSTART, VRAM_TEX_WIDTH, lines);
  Mem_Free(data);
}

static void Mod_LoadSoundData(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_SNDDATA, 0, fh);
  // TODO
}

void Mod_LoadVertexes(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_VERTS, sizeof(xbspvert_t), fh);
  const int numverts = lump.size / sizeof(xbspvert_t);
  mvert_t *out = Mem_Alloc(sizeof(mvert_t) * numverts);
  Sys_FileRead(fh, out, sizeof(xbspvert_t) * numverts); // same struct
  mod->verts = out;
  mod->numverts = numverts;
}

void Mod_LoadPlanes(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_PLANES, sizeof(xbspplane_t), fh);
  const int numplanes = lump.size / sizeof(xbspplane_t);
  mplane_t *out = Mem_Alloc(sizeof(mplane_t) * numplanes);
  mod->planes = out;
  mod->numplanes = numplanes;
  for (int i = 0; i < numplanes; ++i, ++out) {
    xbspplane_t in;
    Sys_FileRead(fh, &in, sizeof(in));
    u8 bits = 0;
    for (int j = 0; j < 3; j++) {
      out->normal.d[j] = in.normal.d[j];
      if (out->normal.d[j] < 0)
        bits |= 1 << j;
    }
    out->dist = in.dist;
    out->type = in.type;
    out->signbits = bits;
  }
}

void Mod_LoadTexinfo(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_TEXINFO, sizeof(xbsptexinfo_t), fh);
  const int numtexinfo = lump.size / sizeof(xbsptexinfo_t);
  mtexture_t *out = Mem_Alloc(sizeof(mtexture_t) * numtexinfo);
  mod->textures = out;
  mod->numtextures = numtexinfo;
  for (int i = 0; i < numtexinfo; ++i, ++out) {
    xbsptexinfo_t in;
    Sys_FileRead(fh, &in, sizeof(in));
    out->width = in.size.x;
    out->height = in.size.y;
    out->vram_page = in.tpage;
    out->vram_u = in.uv.u;
    out->vram_v = in.uv.v;
    out->anim_total = 0;
    out->flags = in.flags;
  }
}

void Mod_LoadFaces(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_FACES, sizeof(xbspface_t), fh);
  const int numfaces = lump.size / sizeof(xbspface_t);
  msurface_t *out = Mem_Alloc(sizeof(msurface_t) * numfaces);
  mod->surfaces = out;
  mod->numsurfaces = numfaces;
  for (int i = 0; i < numfaces; ++i, ++out) {
    xbspface_t in;
    Sys_FileRead(fh, &in, sizeof(in));
    out->flags = 0;
    if (in.side)
      out->flags |= SURF_PLANEBACK;
    out->plane = mod->planes + in.planenum;
    out->texture = mod->textures + in.texinfo;
    out->firstvert = in.firstvert;
    out->numverts = in.numverts;
  }
}

void Mod_LoadMarksurfaces(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_MARKSURF, sizeof(u16), fh);
  const int nummarksurf = lump.size / sizeof(u16);
  msurface_t **out = Mem_Alloc(sizeof(msurface_t *) * nummarksurf);
  mod->marksurfaces = out;
  mod->nummarksurfaces = nummarksurf;
  u16 *in = Mem_Alloc(sizeof(u16) * nummarksurf);
  Sys_FileRead(fh, in, lump.size);
  for (int i = 0; i < nummarksurf; ++i, ++out) {
    if (in[i] >= mod->numsurfaces)
      Sys_Error("Mod_LoadMarksurfaces: bad surface number %d", in[i]);
    *out = mod->surfaces + in[i];
  }
  Mem_Free(in);
}

void Mod_LoadNodes(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_NODES, sizeof(xbspnode_t), fh);
  const int numnodes = lump.size / sizeof(xbspnode_t);
  mnode_t *out = Mem_Alloc(sizeof(mnode_t) * numnodes);
  mod->nodes = out;
  mod->numnodes = numnodes;
  for (int i = 0; i < numnodes; ++i, ++out) {
    xbspnode_t in;
    Sys_FileRead(fh, &in, sizeof(in));
    out->mins = (x32vec3_t){ TO_FIX32(in.mins.x), TO_FIX32(in.mins.y), TO_FIX32(in.mins.z) };
    out->maxs = (x32vec3_t){ TO_FIX32(in.maxs.x), TO_FIX32(in.maxs.y), TO_FIX32(in.maxs.z) };
    out->plane = mod->planes + in.planenum;
    out->firstsurf = in.firstface;
    out->numsurf = in.numfaces;
    for (int j = 0; j < 2; j++) {
      const int p = in.children[j];
      if (p >= 0)
        out->children[j] = mod->nodes + p;
      else
        out->children[j] = (mnode_t *)(mod->leafs + (-1 - p));
    }
  }
  Mod_SetParent(mod->nodes, NULL); // sets nodes and leafs
}

void Mod_LoadClipnodes(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_CLIPNODES, sizeof(xbspclipnode_t), fh);
  const int numclipnodes = lump.size / sizeof(xbspclipnode_t);
  mclipnode_t *out = Mem_Alloc(sizeof(mclipnode_t) * numclipnodes);
  mod->clipnodes = out;
  mod->numclipnodes = numclipnodes;

  Sys_FileRead(fh, out, sizeof(xbspclipnode_t) * numclipnodes); // same struct

  // also init hulls

  hull_t *hull = &mod->hulls[1];
  hull->clipnodes = out;
  hull->firstclipnode = 0;
  hull->lastclipnode = numclipnodes - 1;
  hull->planes = mod->planes;
  hull->mins = (x32vec3_t){ TO_FIX32(-16), TO_FIX32(-16), TO_FIX32(-24) };
  hull->maxs = (x32vec3_t){ TO_FIX32(+16), TO_FIX32(+16), TO_FIX32(+32) };

	hull = &mod->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = numclipnodes - 1;
	hull->planes = mod->planes;
  hull->mins = (x32vec3_t){ TO_FIX32(-32), TO_FIX32(-32), TO_FIX32(-24) };
  hull->maxs = (x32vec3_t){ TO_FIX32(+32), TO_FIX32(+32), TO_FIX32(+64) };
}

void Mod_LoadLeafs(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_LEAFS, sizeof(xbspleaf_t), fh);
  const int numleafs = lump.size / sizeof(xbspleaf_t);
  mleaf_t *out = Mem_Alloc(sizeof(mleaf_t) * numleafs);
  mod->leafs = out;
  mod->numleafs = numleafs;
  for (int i = 0; i < numleafs; ++i, ++out) {
    xbspleaf_t in;
    Sys_FileRead(fh, &in, sizeof(in));
    out->mins = (x32vec3_t){ TO_FIX32(in.mins.x), TO_FIX32(in.mins.y), TO_FIX32(in.mins.z) };
    out->maxs = (x32vec3_t){ TO_FIX32(in.maxs.x), TO_FIX32(in.maxs.y), TO_FIX32(in.maxs.z) };
    out->contents = in.contents;
    out->firstmarksurf = mod->marksurfaces + in.firstmarksurface;
    out->nummarksurf = in.nummarksurfaces;
    out->compressed_vis = in.visofs < 0 ? NULL : mod->visdata + in.visofs;
  }
}

void Mod_LoadVisibility(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_VISILIST, 0, fh);
  if (!lump.size) {
    // no vis information
    mod->visdata = NULL;
    return;
  }
  u8 *out = Mem_Alloc(lump.size);
  mod->visdata = out;
  Sys_FileRead(fh, out, lump.size);
}

void Mod_LoadSubmodels(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_MODELS, sizeof(xbspmodel_t), fh);
  const int nummodels = lump.size / sizeof(xbspmodel_t);
  xbspmodel_t *out = Mem_Alloc(sizeof(xbspmodel_t) * nummodels);
  mod->submodels = out;
  mod->numsubmodels = nummodels;
  Sys_FileRead(fh, out, lump.size); // same struct
}

void Mod_LoadEntities(model_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_ENTITIES, 0, fh);
  // TODO
}

void Mod_MakeHull0(model_t *mod) {
  hull_t *hull = &mod->hulls[0];
  const int count = mod->numnodes;
  mnode_t *in = mod->nodes;
  mclipnode_t *out = Mem_Alloc(sizeof(mclipnode_t) * count);
  hull->clipnodes = out;
  hull->firstclipnode = 0;
  hull->lastclipnode = count - 1;
  hull->planes = mod->planes;
  for (int i = 0; i < count; ++i, ++out, ++in) {
    out->planenum = in->plane - mod->planes;
    for (int j = 0; j < 2; ++j) {
      mnode_t *child = in->children[j];
      if (child->contents < 0)
        out->children[j] = child->contents;
      else
        out->children[j] = child - mod->nodes;
    }
  }
}

void Mod_LoadBrushModel(model_t *mod, const int fh) {
  mod->type = mod_brush;

  xbsphdr_t header;
  Sys_FileRead(fh, &header, sizeof(header));
  if (header.ver != PSXBSPVERSION)
    Sys_Error("Mod_LoadBrushModel: %04x has wrong version number (%d should be %d)\n", mod->id, header.ver, PSXBSPVERSION);

  // fat data comes first, then gets offloaded from RAM to VRAM/SPURAM
  Mod_LoadClutData(mod, fh);
  Mod_LoadTextureData(mod, fh);
  Mod_LoadSoundData(mod, fh);

  // then the map itself
  Mod_LoadVertexes(mod, fh);
  Mod_LoadPlanes(mod, fh);
  Mod_LoadTexinfo(mod, fh);
  Mod_LoadFaces(mod, fh);
  Mod_LoadMarksurfaces(mod, fh);
  Mod_LoadVisibility(mod, fh);
  Mod_LoadLeafs(mod, fh);
  Mod_LoadNodes(mod, fh);
  Mod_LoadClipnodes(mod, fh);
  Mod_LoadSubmodels(mod, fh);
  Mod_LoadEntities(mod, fh);
  Mod_MakeHull0(mod);
}

model_t *Mod_LoadModel(const char *name) {
  int fhandle;
  const int fsize = Sys_FileOpenRead(name, &fhandle);
  if (fsize < 0) Sys_Error("Mod_LoadModel: couldn't open");

  model_t *model = Mem_Alloc(sizeof(model_t));
  Mod_LoadBrushModel(model, fhandle);
  Sys_FileClose(fhandle);

  return model;
}

mleaf_t *Mod_PointInLeaf(const x32vec3_t p, model_t *mod) {
  if (!mod || !mod->nodes)
    Sys_Error("Mod_PointInLeaf: bad model %p", mod);

  mnode_t *node = mod->nodes;

  while (1) {
    if (node->contents < 0)
      return (mleaf_t *)node;
    const mplane_t *plane = node->plane;
    const x32 d = XVecDotSL(plane->normal, p) - plane->dist;
    if (d > 0)
      node = node->children[0];
    else
      node = node->children[1];
  }

  return NULL;	// never reached
}

static inline const u8 *Mod_DecompressVis(const u8 *in, const model_t *model) {
  // we can decompress into scratch since nothing else is using it when we do
  // and it fits the size exactly (1kb)
  static u8 *decompressed = PSX_SCRATCH;
  int c;
  u8 *out;
  int row;

  row = (model->numleafs + 7) >> 3;	
  out = decompressed;

  if (!in) {
    // no vis info, so make all visible
    while (row) {
      *out++ = 0xff;
      row--;
    }
    return decompressed;
  }

  do {
    if (*in) {
      *out++ = *in++;
      continue;
    }
    c = in[1];
    in += 2;
    while (c) {
      *out++ = 0;
      c--;
    }
  } while (out - decompressed < row);

  return decompressed;
}

const u8 *Mod_LeafPVS(const mleaf_t *leaf, const model_t *model) {
  static const u8 novis[MAX_MAP_LEAFS / 8] = { 0xFF };
  if (leaf == model->leafs)
    return novis;
  return Mod_DecompressVis(leaf->compressed_vis, model);
}
