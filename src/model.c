#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system.h"
#include "bspfile.h"
#include "render.h"
#include "model.h"

static x32 RadiusFromBounds(const x32vec3_t *mins, const x32vec3_t *maxs) {
  x32vec3_t corner;
  for (int i = 0; i < 3; i++)
    corner.d[i] = abs(mins->d[i]) > abs(maxs->d[i]) ? abs(mins->d[i]) : abs(maxs->d[i]);
  return XVecLengthL(&corner);
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

static void Mod_LoadClutData(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_CLUTDATA, VID_NUM_COLORS * 2, fh);
  u16 clut[VID_NUM_COLORS];
  Sys_FileRead(fh, clut, VID_NUM_COLORS * 2);
  R_UploadClut(clut);
}

static void Mod_LoadTextureData(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_TEXDATA, 0, fh);
  const int lines = lump.size / (VRAM_TEX_WIDTH * 2);
  ASSERT(lines <= VRAM_TEX_HEIGHT);
  u8 *data = Mem_Alloc(lump.size);
  Sys_FileRead(fh, data, lump.size);
  R_UploadTexture(data, VRAM_TEX_XSTART, VRAM_TEX_YSTART, VRAM_TEX_WIDTH, lines);
  Mem_Free(data);
}

static void Mod_LoadSoundData(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_SNDDATA, 0, fh);
  // TODO
}

static void Mod_LoadAliasData(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_MDLDATA, 0, fh);

  u32 num_mdls = 0;
  Sys_FileRead(fh, &num_mdls, sizeof(u32));

  amodel_t *mdls = Mem_Alloc(lump.size - sizeof(u32));
  Sys_FileRead(fh, mdls, lump.size - sizeof(u32)); // same structs

  u8 *mdldata = (u8 *)(mdls + num_mdls);

  // fixup offsets
  for (u32 i = 0; i < num_mdls; ++i) {
    mdls[i].frames = (void *)((uintptr_t)mdls[i].frames + (u8 *)mdldata);
    mdls[i].tris = (void *)((uintptr_t)mdls[i].tris + (u8 *)mdldata);
    mdls[i].texcoords = (void *)((uintptr_t)mdls[i].texcoords + (u8 *)mdldata);
  }

  mod->numamodels = num_mdls;
  mod->amodels = mdls;
}

void Mod_LoadVertexes(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_VERTS, sizeof(xbspvert_t), fh);
  const int numverts = lump.size / sizeof(xbspvert_t);
  mvert_t *out = Mem_Alloc(sizeof(mvert_t) * numverts);
  Sys_FileRead(fh, out, sizeof(xbspvert_t) * numverts); // same struct
  mod->verts = out;
  mod->numverts = numverts;
}

void Mod_LoadPlanes(bmodel_t *mod, const int fh) {
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

void Mod_LoadTexinfo(bmodel_t *mod, const int fh) {
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
    out->texchain = NULL;
  }
}

void Mod_LoadFaces(bmodel_t *mod, const int fh) {
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
    out->styles[0] = in.styles[0];
    out->styles[1] = in.styles[1];
    out->texchain = NULL;
    out->visframe = 0;
  }
}

void Mod_LoadMarksurfaces(bmodel_t *mod, const int fh) {
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

void Mod_LoadNodes(bmodel_t *mod, const int fh) {
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
    out->visframe = 0;
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

void Mod_LoadClipnodes(bmodel_t *mod, const int fh) {
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

void Mod_LoadLeafs(bmodel_t *mod, const int fh) {
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
    out->visframe = 0;
  }
}

void Mod_LoadVisibility(bmodel_t *mod, const int fh) {
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

void Mod_LoadSubmodels(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_MODELS, sizeof(xmodel_t), fh);
  const int nummodels = lump.size / sizeof(xmodel_t);

  // allocate space for the extra bmodel_t array first, since they're not gonna be freed until later
  mod->bmodels = Mem_ZeroAlloc(sizeof(bmodel_t) * (nummodels - 1));
  mod->bmodelptrs = Mem_ZeroAlloc(sizeof(bmodel_t*) * nummodels);

  xmodel_t *out = Mem_Alloc(sizeof(xmodel_t) * nummodels);
  mod->submodels = out;
  mod->numsubmodels = nummodels;
  Sys_FileRead(fh, out, lump.size); // same struct
}

void Mod_LoadEntities(bmodel_t *mod, const int fh) {
  INIT_LUMP(lump, LUMP_ENTITIES, sizeof(xbspent_t), fh);
  const int numents = lump.size / sizeof(xbspent_t);
  xbspent_t *out = Mem_Alloc(sizeof(xbspent_t) * numents);
  mod->mapents = out;
  mod->nummapents = numents;
  Sys_FileRead(fh, out, lump.size); // same struct
}

bmodel_t **Mod_SetupSubmodels(bmodel_t *worldmodel) {
  const int numsubmodels = worldmodel->numsubmodels;
  bmodel_t **submodelptrs = worldmodel->bmodelptrs;
  submodelptrs[0] = worldmodel;
  if (numsubmodels == 1) {
    // don't need these after this is done
    Mem_Free(worldmodel->submodels);
    worldmodel->submodels = NULL;
    return submodelptrs;
  }

  bmodel_t *submodels = worldmodel->bmodels;
  for (int i = 1; i < numsubmodels; ++i)
    submodelptrs[i] = &submodels[i - 1];

  for (int i = 0; i < numsubmodels; ++i) {
    bmodel_t *mod = submodelptrs[i];
    xmodel_t *bm = &worldmodel->submodels[i];

    // duplicate the basic information
    *mod = *worldmodel;

    mod->type = mod_brush;
    mod->id = -i;

    mod->hulls[0].firstclipnode = bm->headnode[0];
    for (int j = 1; j < MAX_MAP_HULLS; ++j) {
      mod->hulls[j].firstclipnode = bm->headnode[j];
      mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
    }

    mod->firstmodelsurface = bm->firstface;
    mod->nummodelsurfaces = bm->numfaces;

    mod->maxs.x = TO_FIX32(bm->maxs.x);
    mod->maxs.y = TO_FIX32(bm->maxs.y);
    mod->maxs.z = TO_FIX32(bm->maxs.z);

    mod->mins.x = TO_FIX32(bm->mins.x);
    mod->mins.y = TO_FIX32(bm->mins.y);
    mod->mins.z = TO_FIX32(bm->mins.z);

    mod->radius = RadiusFromBounds(&mod->mins, &mod->maxs);

    mod->numleafs = bm->visleafs;

    if (i) {
      // submodels don't have submodels
      mod->submodels = NULL;
      mod->bmodels = NULL;
      mod->bmodelptrs = NULL;
      mod->numsubmodels = 0;
    }
  }

  // don't need these after this is done
  Mem_Free(worldmodel->submodels);
  worldmodel->submodels = NULL;

  return submodelptrs;
}

void Mod_MakeHull0(bmodel_t *mod) {
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

static void Mod_ParseBrushModel(bmodel_t *mod, const int fh) {
  mod->type = mod_brush;

  xbsphdr_t header;
  Sys_FileRead(fh, &header, sizeof(header));
  if (header.ver != PSXBSPVERSION)
    Sys_Error("Mod_LoadBrushModel: %04x has wrong version number (%d should be %d)\n", mod->id, header.ver, PSXBSPVERSION);

  // fat data comes first, then gets offloaded from RAM to VRAM/SPURAM
  Mod_LoadClutData(mod, fh);
  Mod_LoadTextureData(mod, fh);
  Mod_LoadSoundData(mod, fh);
  Mod_LoadAliasData(mod, fh);

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
  Mod_SetupSubmodels(mod);
  Mod_MakeHull0(mod);

  // load entities last so we can free the data after parsing it
  Mod_LoadEntities(mod, fh);
}

bmodel_t *Mod_LoadXBSP(const char *name) {
  int fhandle;
  const int fsize = Sys_FileOpenRead(name, &fhandle);
  if (fsize < 0) Sys_Error("Mod_LoadModel: couldn't open");

  bmodel_t *model = Mem_Alloc(sizeof(bmodel_t));
  Mod_ParseBrushModel(model, fhandle);
  Sys_FileClose(fhandle);

  return model;
}

mleaf_t *Mod_PointInLeaf(const x32vec3_t *p, bmodel_t *mod) {
  if (!mod || !mod->nodes)
    Sys_Error("Mod_PointInLeaf: bad model %p", mod);

  mnode_t *node = mod->nodes;

  while (1) {
    if (node->contents < 0)
      return (mleaf_t *)node;
    const mplane_t *plane = node->plane;
    const x32 d = XVecDotSL(&plane->normal, p) - plane->dist;
    if (d > 0)
      node = node->children[0];
    else
      node = node->children[1];
  }

  return NULL; // never reached
}

static inline const u8 *Mod_DecompressVis(const u8 *in, const bmodel_t *model) {
  // we can decompress into scratch since nothing else is using it when we do
  // and it fits the size exactly (1kb)
  static u8 *decompressed = PSX_SCRATCH;
  int c;
  u8 *out;
  int row;

  row = (model->numleafs + 7) >> 3;
  out = decompressed;

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

const u8 *Mod_LeafPVS(const mleaf_t *leaf, const bmodel_t *model) {
  if (leaf == model->leafs || !leaf->compressed_vis) {
    // nothing is visible
    memset(PSX_SCRATCH, 0, 1024);
    return PSX_SCRATCH;
  }
  return Mod_DecompressVis(leaf->compressed_vis, model);
}
