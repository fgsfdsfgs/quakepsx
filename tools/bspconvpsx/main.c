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
#include "qbsp.h"
#include "qmdl.h"

static const char *moddir;
static const char *inname;
static const char *outname;
static const char *vramexp;

static void cleanup(void) {
  if (qbsp.start) free(qbsp.start);
  if (qbsp.palette) free(qbsp.palette);
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
  const int numtex = qbsp.miptex->nummiptex;

  if (numtex > MAX_XMAP_TEXTURES)
    panic("input map has too many textures (%d > %d)", numtex, MAX_XMAP_TEXTURES);

  // sort textures by size, largest first
  struct texsort sorted[numtex];
  for (int i = 0; i < numtex; ++i) {
    sorted[i] = (struct texsort) {
      .qtex = qbsp_get_miptex(&qbsp, i),
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
    xbsp_vram_export(export, qbsp.palette);
}

static void do_planes(void) {
  printf("converting %d planes\n", qbsp.numplanes);

  for (int i = 0; i < qbsp.numplanes; ++i) {
    xplane_t *xp = xbsp_planes + i;
    xp->type = qbsp.planes[i].type;
    xp->dist = f32_to_x32(qbsp.planes[i].dist * XBSP_SCALE);
    xp->normal = qvec3_to_x16vec3(qbsp.planes[i].normal);
  }

  xbsp_numplanes = qbsp.numplanes;
  xbsp_lumps[XLMP_PLANES].size = sizeof(xplane_t) * qbsp.numplanes;
}

static void do_faces(void) {
  printf("converting %d faces\n", qbsp.numfaces);

  for (int f = 0; f < qbsp.numfaces; ++f) {
    const qface_t *qf = qbsp.faces + f;
    xface_t *xf = xbsp_faces + f;
    xf->planenum = qf->planenum;
    xf->side = qf->side;
    xbsp_face_add(xf, qf, &qbsp);
    printf("* qface %05d numverts %03d -> %03d\n", f, qf->numedges, xf->numverts);
  }

  xbsp_lumps[XLMP_FACES].size = xbsp_numfaces * sizeof(xface_t);
  xbsp_lumps[XLMP_VERTS].size = xbsp_numverts * sizeof(xvert_t);

  printf("* added %d/%d vertices in %d faces\n", xbsp_numverts, MAX_XMAP_VERTS, xbsp_numfaces);
}

static void do_visdata(void) {
  xbsp_numvisdata = xbsp_lumps[XLMP_VISILIST].size = qbsp.numvisdata;
  memcpy(xbsp_visdata, qbsp.visdata, qbsp.numvisdata);
  printf("visdata size: %d\n", qbsp.numvisdata);
}

static void do_nodes(void) {
  printf("converting %d nodes, %d clipnodes, %d leaves, %d marksurfaces\n",
    qbsp.numnodes, qbsp.numcnodes, qbsp.numleafs, qbsp.nummarksurf);

  for (int i = 0; i < qbsp.numnodes; ++i) {
    const qnode_t *qn = qbsp.nodes + i;
    xnode_t *xn = xbsp_nodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->firstface = qn->firstface;
    xn->numfaces = qn->numfaces;
    xn->planenum = qn->planenum;
    xn->mins.d[0] = qn->mins[0] * XBSP_SCALE;
    xn->mins.d[1] = qn->mins[1] * XBSP_SCALE;
    xn->mins.d[2] = qn->mins[2] * XBSP_SCALE;
    xn->maxs.d[0] = qn->maxs[0] * XBSP_SCALE;
    xn->maxs.d[1] = qn->maxs[1] * XBSP_SCALE;
    xn->maxs.d[2] = qn->maxs[2] * XBSP_SCALE;
  }

  for (int i = 0; i < qbsp.numcnodes; ++i) {
    const qclipnode_t *qn = qbsp.cnodes + i;
    xclipnode_t *xn = xbsp_clipnodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->planenum = qn->planenum;
  }

  for (int i = 0; i < qbsp.numleafs; ++i) {
    const qleaf_t *ql = qbsp.leafs + i;
    xleaf_t *xl = xbsp_leafs + i;
    xl->contents = ql->contents;
    xl->visofs = ql->visofs;
    xl->firstmarksurface = ql->firstmarksurface;
    xl->nummarksurfaces = ql->nummarksurfaces;
    xl->mins.d[0] = ql->mins[0] * XBSP_SCALE;
    xl->mins.d[1] = ql->mins[1] * XBSP_SCALE;
    xl->mins.d[2] = ql->mins[2] * XBSP_SCALE;
    xl->maxs.d[0] = ql->maxs[0] * XBSP_SCALE;
    xl->maxs.d[1] = ql->maxs[1] * XBSP_SCALE;
    xl->maxs.d[2] = ql->maxs[2] * XBSP_SCALE;
  }

  memcpy(xbsp_marksurfs, qbsp.marksurf, qbsp.nummarksurf * 2);

  xbsp_nummarksurfs = qbsp.nummarksurf;
  xbsp_numnodes = qbsp.numnodes;
  xbsp_numclipnodes = qbsp.numcnodes;
  xbsp_numleafs = qbsp.numleafs;
  xbsp_lumps[XLMP_NODES].size = qbsp.numnodes * sizeof(xnode_t);
  xbsp_lumps[XLMP_CLIPNODES].size = qbsp.numcnodes * sizeof(xclipnode_t);
  xbsp_lumps[XLMP_LEAFS].size = qbsp.numleafs * sizeof(xleaf_t);
  xbsp_lumps[XLMP_MARKSURF].size = qbsp.nummarksurf * 2;
}

static void do_models(void) {
  printf("converting %d BSP models\n", qbsp.nummodels);

  for (int i = 0; i < qbsp.nummodels; ++i) {
    const qmodel_t *qm = qbsp.models + i;
    xmodel_t *xm = xbsp_models + i;
    xm->firstface = qm->firstface;
    xm->numfaces = qm->numfaces;
    xm->visleafs = qm->visleafs;
    xm->headnode[0] = qm->headnode[0];
    xm->headnode[1] = qm->headnode[1];
    xm->headnode[2] = qm->headnode[2];
    xm->headnode[3] = qm->headnode[3];
    xm->mins.d[0] = qm->mins[0] * XBSP_SCALE;
    xm->mins.d[1] = qm->mins[1] * XBSP_SCALE;
    xm->mins.d[2] = qm->mins[2] * XBSP_SCALE;
    xm->maxs.d[0] = qm->maxs[0] * XBSP_SCALE;
    xm->maxs.d[1] = qm->maxs[1] * XBSP_SCALE;
    xm->maxs.d[2] = qm->maxs[2] * XBSP_SCALE;
  }

  xbsp_nummodels = qbsp.nummodels;
  xbsp_lumps[XLMP_MODELS].size = qbsp.nummodels * sizeof(xmodel_t);
}

static void do_entmodel_mdl(const char *name, u8 *data, const size_t size) {
  qmdl_t qmdl;
  qmdl_init(&qmdl, data, size);
  printf("* model '%s':\n", name);
  printf("* * numverts  = %d\n", qmdl.header->numverts);
  printf("* * numtris   = %d\n", qmdl.header->numtris);
  printf("* * numframes = %d\n", qmdl.header->numframes);
  printf("* * numskins  = %d\n", qmdl.header->numskins);
  printf("* * skinsize  = %dx%d\n", qmdl.header->skinwidth, qmdl.header->skinheight);
  printf("* * scale     = %f %f %f\n", qmdl.header->scale[0], qmdl.header->scale[1], qmdl.header->scale[2]);
  printf("* * size      = %f\n", qmdl.header->size);

}

static void do_entmodels(void) {
  printf("adding entity models\n");

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

  moddir = argv[1];
  inname = argv[2];
  outname = argv[3];
  vramexp = get_arg(argc, argv, "--export-vram");

  // read palette first
  size_t palsize = 0;
  qbsp.palette = lmp_read(moddir, "gfx/palette.lmp", &palsize);
  if (palsize < (NUM_CLUT_COLORS * 3))
    panic("palette size < %d", NUM_CLUT_COLORS * 3);
  xbsp_set_palette(qbsp.palette);

  // load input map
  size_t bspsize = 0;
  u8 *bsp = lmp_read(moddir, inname, &bspsize);
  qbsp_init(&qbsp, bsp, bspsize);

  // prepare lump headers
  for (int i = 0; i < XLMP_COUNT; ++i)
    xbsp_lumps[i].type = i;

  do_textures(vramexp);
  do_planes();
  do_faces();
  do_visdata();
  do_nodes();
  do_models();
  do_entmodels();

  if (xbsp_write(outname) != 0)
    panic("could not write PSX BSP to '%s'", outname);

  return 0;
}
