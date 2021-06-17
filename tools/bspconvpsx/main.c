#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/psxbsp.h"
#include "../common/util.h"
#include "../common/pak.h"

#include "util.h"
#include "xbsp.h"

static qbsp_t *qbsp_header;
static u8 *qbsp_start;
static size_t qbsp_size;
static u8 *qpalette;

static void cleanup(void) {
  if (qbsp_start) free(qbsp_start);
  if (qpalette) free(qpalette);
}

struct texsort { const qmiptex_t *qtex; int id; };

static int tex_sort(const struct texsort *a, const struct texsort *b) {
  if (a->qtex == NULL) {
    return 1;
  } else if (b->qtex == NULL) {
    return -1;
  } else if (a->qtex->height > b->qtex->height) {
    return -1;
  } else if (a->qtex->height == b->qtex->height) {
    if (b->qtex->width == a->qtex->width)
      return a->id - b->id;
    else
      return b->qtex->width - a->qtex->width;
  } else {
    return 1;
  }
}

static inline void do_textures(const char *export) {
  const u8 *qtexbase = qbsp_start + qbsp_header->miptex.ofs;
  const qmiptexlump_t *qtexhdr = (const qmiptexlump_t *)qtexbase;
  const int numtex = qtexhdr->nummiptex;

  if (numtex > MAX_XMAP_TEXTURES)
    panic("input map has too many textures (%d > %d)", numtex, MAX_XMAP_TEXTURES);

  // sort textures by size, largest first
  struct texsort sorted[numtex];
  for (int i = 0; i < numtex; ++i) {
    sorted[i] = (struct texsort) {
      .qtex = (qtexhdr->dataofs[i] >= 0) ?
        (const qmiptex_t *)(qtexbase + qtexhdr->dataofs[i]) :
        NULL,
      .id = i
    };
  }
  qsort(sorted, numtex, sizeof(struct texsort), (void *)tex_sort);

  printf("converting %d textures:\n", numtex);

  for (int i = 0; i < numtex; ++i) {
    const qmiptex_t *qtex = sorted[i].qtex;
    xtexinfo_t xti = { .flags = 0 };
    xti.flags |= xbsp_texture_flags(qtex);
    if ((xti.flags & XTEX_INVISIBLE) == 0) {
      printf("* %03d '%s' (%dx%d)\n", sorted[i].id, qtex->name, qtex->width, qtex->height);
      int vrx = 0;
      int vry = 0;
      if (xbsp_vram_fit(qtex, &xti, &vrx, &vry))
        panic("VRAM atlas can't fit '%s'", qtex->name);
      xbsp_vram_store(qtex, vrx, vry);
    }
    // store texinfo in normal order
    xbsp_texinfos[sorted[i].id] = xti;
  }

  xbsp_numtexinfos = numtex;
  xbsp_lumps[XLMP_TEXINFO].size = numtex * sizeof(xtexinfo_t);
  xbsp_lumps[XLMP_TEXDATA].size = 2 * VRAM_TOTAL_WIDTH * xbsp_vram_height();
  xbsp_lumps[XLMP_CLUTDATA].size = 2 * NUM_CLUT_COLORS;

  if (export && *export)
    xbsp_vram_export(export, qpalette);
}

static void do_planes(void) {
  const int numplanes = qbsp_header->planes.len / sizeof(qplane_t);
  const qplane_t *planes = (const qplane_t *)(qbsp_start + qbsp_header->planes.ofs);

  printf("converting %d planes\n", numplanes);

  for (int i = 0; i < numplanes; ++i) {
    xplane_t *xp = xbsp_planes + i;
    xp->type = planes[i].type;
    xp->dist = f32_to_x32(planes[i].dist);
    xp->normal = qvec3_to_x16vec3(planes[i].normal);
  }

  xbsp_numplanes = numplanes;
  xbsp_lumps[XLMP_PLANES].size = sizeof(xplane_t) * numplanes;
}

static void do_faces(void) {
  static const qmiptex_t empty;
  const int numfaces = qbsp_header->faces.len / sizeof(qface_t);
  const qface_t *faces = (const qface_t *)(qbsp_start + qbsp_header->faces.ofs);
  const s32 *ledges = (const s32 *)(qbsp_start + qbsp_header->ledges.ofs);
  const qedge_t *edges = (const qedge_t *)(qbsp_start + qbsp_header->edges.ofs);
  const qtexinfo_t *texinfos = (const qtexinfo_t *)(qbsp_start + qbsp_header->texinfo.ofs);
  const u8 *qtexbase = qbsp_start + qbsp_header->miptex.ofs;
  const qmiptexlump_t *qtexhdr = (const qmiptexlump_t *)qtexbase;
  const qvert_t *verts = (const qvert_t *)(qbsp_start + qbsp_header->vertices.ofs);

  printf("converting %d faces\n", numfaces);

  for (int f = 0; f < numfaces; ++f) {
    const qface_t *qf = faces + f;
    const qtexinfo_t *qti = texinfos + qf->texinfo;
    const qmiptex_t *qmt = qtexhdr->dataofs[qti->miptex] >= 0 ?
      (const qmiptex_t *)(qtexbase + qtexhdr->dataofs[qti->miptex]) :
      &empty;
    xface_t *xf = xbsp_faces + f;
    xf->planenum = qf->planenum;
    xf->side = qf->side;
    xbsp_face_add(xf, qf, qti, qmt, ledges, edges, verts);
  }

  xbsp_lumps[XLMP_FACES].size = xbsp_numfaces * sizeof(xface_t);
  xbsp_lumps[XLMP_VERTS].size = xbsp_numverts * sizeof(xvert_t);

  printf("* added %d/%d vertices in %d faces\n", xbsp_numverts, MAX_XMAP_VERTS, xbsp_numfaces);
}

static void do_visdata(void) {
  const int numvisdata = qbsp_header->visilist.len;
  const u8 *visdata = qbsp_start + qbsp_header->visilist.ofs;
  xbsp_numvisdata = xbsp_lumps[XLMP_VISILIST].size = numvisdata;
  memcpy(xbsp_visdata, visdata, numvisdata);
  printf("visdata size: %d\n", numvisdata);
}

static void do_nodes(void) {
  const int numnodes = qbsp_header->nodes.len / sizeof(qnode_t);
  const qnode_t *nodes = (const qnode_t *)(qbsp_start + qbsp_header->nodes.ofs);
  const int numcnodes = qbsp_header->clipnodes.len / sizeof (qclipnode_t);
  const qclipnode_t *cnodes = (const qclipnode_t *)(qbsp_start + qbsp_header->clipnodes.ofs);
  const int numleafs = qbsp_header->leafs.len / sizeof (qleaf_t);
  const qleaf_t *leafs = (const qleaf_t *)(qbsp_start + qbsp_header->leafs.ofs);
  const int nummarksurf = qbsp_header->marksurfaces.len / 2;
  const u16 *marksurf = (const u16 *)(qbsp_start + qbsp_header->marksurfaces.ofs);

  printf("converting %d nodes, %d clipnodes, %d leaves, %d marksurfaces\n", numnodes, numcnodes, numleafs, nummarksurf);

  for (int i = 0; i < numnodes; ++i) {
    const qnode_t *qn = nodes + i;
    xnode_t *xn = xbsp_nodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->firstface = qn->firstface;
    xn->numfaces = qn->numfaces;
    xn->planenum = qn->planenum;
    xn->mins.d[0] = qn->mins[0];
    xn->mins.d[1] = qn->mins[1];
    xn->mins.d[2] = qn->mins[2];
    xn->maxs.d[0] = qn->maxs[0];
    xn->maxs.d[1] = qn->maxs[1];
    xn->maxs.d[2] = qn->maxs[2];
  }

  for (int i = 0; i < numcnodes; ++i) {
    const qclipnode_t *qn = cnodes + i;
    xclipnode_t *xn = xbsp_clipnodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->planenum = qn->planenum;
  }

  for (int i = 0; i < numleafs; ++i) {
    const qleaf_t *ql = leafs + i;
    xleaf_t *xl = xbsp_leafs + i;
    xl->contents = ql->contents;
    xl->visofs = ql->visofs;
    xl->firstmarksurface = ql->firstmarksurface;
    xl->nummarksurfaces = ql->nummarksurfaces;
    xl->mins.d[0] = ql->mins[0];
    xl->mins.d[1] = ql->mins[1];
    xl->mins.d[2] = ql->mins[2];
    xl->maxs.d[0] = ql->maxs[0];
    xl->maxs.d[1] = ql->maxs[1];
    xl->maxs.d[2] = ql->maxs[2];
  }

  memcpy(xbsp_marksurfs, marksurf, nummarksurf * 2);

  xbsp_nummarksurfs = nummarksurf;
  xbsp_numnodes = numnodes;
  xbsp_numclipnodes = numcnodes;
  xbsp_numleafs = numleafs;
  xbsp_lumps[XLMP_NODES].size = numnodes * sizeof(xnode_t);
  xbsp_lumps[XLMP_CLIPNODES].size = numcnodes * sizeof(xclipnode_t);
  xbsp_lumps[XLMP_LEAFS].size = numleafs * sizeof(xleaf_t);
  xbsp_lumps[XLMP_MARKSURF].size = nummarksurf * 2;
}

static void do_models(void) {
  const int nummodels = qbsp_header->models.len / sizeof(qmodel_t);
  const qmodel_t *models = (const qmodel_t *)(qbsp_start + qbsp_header->models.ofs);

  printf("converting %d BSP models\n", nummodels);

  for (int i = 0; i < nummodels; ++i) {
    const qmodel_t *qm = models + i;
    xmodel_t *xm = xbsp_models + i;
    xm->firstface = qm->firstface;
    xm->numfaces = qm->numfaces;
    xm->visleafs = qm->visleafs;
    xm->headnode[0] = qm->headnode[0];
    xm->headnode[1] = qm->headnode[1];
    xm->headnode[2] = qm->headnode[2];
    xm->headnode[3] = qm->headnode[3];
    xm->mins.d[0] = qm->mins[0];
    xm->mins.d[1] = qm->mins[1];
    xm->mins.d[2] = qm->mins[2];
    xm->maxs.d[0] = qm->maxs[0];
    xm->maxs.d[1] = qm->maxs[1];
    xm->maxs.d[2] = qm->maxs[2];
  }

  xbsp_nummodels = nummodels;
  xbsp_lumps[XLMP_MODELS].size = nummodels * sizeof(xmodel_t);
}

static inline const char *get_arg(int c, const char **v, const char *arg) {
  for (int i = 4; i < c; ++i) {
    if (!strcmp(v[i], arg)) {
      if (i == c - 1)
        return "";
      else
        return v[i + 1];
    }
  }
  return NULL;
}

int main(int argc, const char **argv) {
  if (argc < 4) {
    printf("usage: %s <moddir> <mapname> <outxbsp> [<options>]\n", argv[0]);
    return -1;
  }

  atexit(cleanup);

  const char *moddir = argv[1];
  const char *inname = argv[2];
  const char *outname = argv[3];
  const char *vramexp = get_arg(argc, argv, "--export-vram");

  // read palette first
  size_t palsize = 0;
  qpalette = lmp_read(moddir, "gfx/palette.lmp", &palsize);
  if (palsize < (NUM_CLUT_COLORS * 3))
    panic("palette size < %d", NUM_CLUT_COLORS * 3);
  xbsp_set_palette(qpalette);

  // load input map
  qbsp_start = lmp_read(moddir, inname, &qbsp_size);
  if (qbsp_size < sizeof(*qbsp_header))
    panic("malformed Quake BSP: size < %d", sizeof(*qbsp_header));
  qbsp_header = (qbsp_t *)qbsp_start;
  if (qbsp_header->ver != QBSPVERSION)
    panic("malformed Quake BSP: ver != %d", QBSPVERSION);

  // prepare lump headers
  for (int i = 0; i < XLMP_COUNT; ++i)
    xbsp_lumps[i].type = i;

  do_textures(vramexp);
  do_planes();
  do_faces();
  do_visdata();
  do_nodes();
  do_models();

  if (xbsp_write(outname) != 0)
    panic("could not write PSX BSP to '%s'", outname);

  return 0;
}
