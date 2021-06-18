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

  return 0;
}

const qmiptex_t *qbsp_get_miptex(const qbsp_t *qbsp, const int i) {
  return (qbsp->miptex->dataofs[i] >= 0) ?
    (const qmiptex_t *)((const u8 *)qbsp->miptex + qbsp->miptex->dataofs[i]) :
    NULL;
}
