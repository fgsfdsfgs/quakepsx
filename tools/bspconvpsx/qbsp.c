#include <stdlib.h>
#include "qbsp.h"
#include "util.h"
#include "../common/psxtypes.h"
#include "../common/idbsp.h"

qbsp_t qbsp;

int qbsp_init(qbsp_t *qbsp, u8 *bsp, const size_t size) {
  qbsp->start = bsp;
  qbsp->size = size;

  if (qbsp->size < sizeof(*qbsp->header))
    panic("malformed Quake BSP: size < %d", sizeof(*qbsp->header));

  qbsp->header = (qbsphdr_t *)qbsp->start;
  if (qbsp->header->ver != QBSPVERSION)
    panic("malformed Quake BSP: ver != %d", QBSPVERSION);

  // set lump pointers

  qbsp->numplanes =qbsp->header->planes.len / sizeof(qplane_t);
  qbsp->planes = (qplane_t *)(qbsp->start + qbsp->header->planes.ofs);

  qbsp->numnodes = qbsp->header->nodes.len / sizeof(qnode_t);
  qbsp->nodes = (qnode_t *)(qbsp->start + qbsp->header->nodes.ofs);

  qbsp->numcnodes = qbsp->header->clipnodes.len / sizeof (qclipnode_t);
  qbsp->cnodes = (qclipnode_t *)(qbsp->start + qbsp->header->clipnodes.ofs);

  qbsp->numleafs = qbsp->header->leafs.len / sizeof (qleaf_t);
  qbsp->leafs = (qleaf_t *)(qbsp->start + qbsp->header->leafs.ofs);

  qbsp->nummarksurf = qbsp->header->marksurfaces.len / sizeof(u16);
  qbsp->marksurf = (u16 *)(qbsp->start + qbsp->header->marksurfaces.ofs);

  qbsp->numvisdata = qbsp->header->visilist.len;
  qbsp->visdata = qbsp->start + qbsp->header->visilist.ofs;

  qbsp->numfaces = qbsp->header->faces.len / sizeof(qface_t);
  qbsp->faces = (qface_t *)(qbsp->start + qbsp->header->faces.ofs);

  qbsp->numsurfedges = qbsp->header->surfedges.len / sizeof(s32);
  qbsp->surfedges = (s32 *)(qbsp->start + qbsp->header->surfedges.ofs);

  qbsp->numedges = qbsp->header->edges.len / sizeof(qedge_t);
  qbsp->edges = (qedge_t *)(qbsp->start + qbsp->header->edges.ofs);

  qbsp->numverts = qbsp->header->vertices.len / sizeof(qvert_t);
  qbsp->verts = (qvert_t *)(qbsp->start + qbsp->header->vertices.ofs);

  qbsp->numtexinfos = qbsp->header->texinfo.len / sizeof(qtexinfo_t);
  qbsp->texinfos = (qtexinfo_t *)(qbsp->start + qbsp->header->texinfo.ofs);

  qbsp->nummodels = qbsp->header->models.len / sizeof(qmodel_t);
  qbsp->models = (qmodel_t *)(qbsp->start + qbsp->header->models.ofs);

  qbsp->miptex = (qmiptexlump_t *)(qbsp->start + qbsp->header->miptex.ofs);

  qbsp->numlightdata = qbsp->header->lightmaps.len;
  qbsp->lightdata = qbsp->start + qbsp->header->lightmaps.ofs;

  return 0;
}

const qmiptex_t *qbsp_get_miptex(const qbsp_t *qbsp, const int i) {
  return (qbsp->miptex->dataofs[i] >= 0) ?
    (const qmiptex_t *)((const u8 *)qbsp->miptex + qbsp->miptex->dataofs[i]) :
    NULL;
}

u16 qbsp_light_for_vert(const qbsp_t *qbsp, const qface_t *qf, const qvec3_t v, qvec2_t sorg, qvec2_t sext, u16 *out) {
  if (qf->lightofs < 0) {
    // no lightmap => fullbright
    out[0] = out[1] = out[2] = out[3] = 0x40;
    return 0x40;
  }

  const qtexinfo_t *qti = qbsp->texinfos + qf->texinfo;
  const qmiptex_t *qmt = qbsp_get_miptex(qbsp, qti->miptex);
  if (qti->flags & TEXF_SPECIAL) {
    // this is water or sky or some shit => fullbright
    out[0] = out[1] = out[2] = out[3] = 0x40;
    return 0x40;
  }

  const u8 *samples = qbsp->lightdata + qf->lightofs;

  // don't have to check whether the point is on the surface, since we already know it is
  const int s = qdot3(v, qti->vecs[0]) + qti->vecs[0][3];
  const int t = qdot3(v, qti->vecs[1]) + qti->vecs[1][3];
  int ds = s - (int)sorg[0];
  int dt = t - (int)sorg[1];

  if (ds < 0) ds = 0;
  if (ds > sext[0]) ds = sext[0];
  if (dt < 0) dt = 0;
  if (dt > sext[1]) dt = sext[1];

  ds >>= 4;
  dt >>= 4;

  u32 r = 0;
  u32 scale = 255; // og max light is 264, but for us anything above 128 is overbright
  samples += dt * (((int)sext[0] >> 4) + 1) + ds;
  for (int m = 0; m < MAX_LIGHTMAPS && qf->styles[m] != 255; m++) {
    const u32 v = *samples * scale;
    r += v;
    out[m] = v >> 8;
    samples += (((int)sext[0]>>4)+1) * (((int)sext[1]>>4)+1);
  }
  return r >> 8;
}
