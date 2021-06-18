#pragma once

#include <stdlib.h>
#include "../common/psxtypes.h"
#include "../common/idbsp.h"

typedef struct {
  u8 *start;
  size_t size;

  qbsphdr_t *header;

  int numplanes;
  qplane_t *planes;

  int numnodes;
  qnode_t *nodes;

  int numcnodes;
  qclipnode_t *cnodes;

  int numleafs;
  qleaf_t *leafs;

  int nummarksurf;
  u16 *marksurf;

  int numvisdata;
  u8 *visdata;

  int numfaces;
  qface_t *faces;

  int numsurfedges;
  s32 *surfedges;

  int numedges;
  qedge_t *edges;

  int numtexinfos;
  qtexinfo_t *texinfos;

  int numverts;
  qvert_t *verts;

  int nummodels;
  qmodel_t *models;

  int numlightdata;
  u8 *lightdata;

  qmiptexlump_t *miptex;

  u8 *palette;
} qbsp_t;

extern qbsp_t qbsp;

int qbsp_init(qbsp_t *qbsp, u8 *bsp, const size_t size);
const qmiptex_t *qbsp_get_miptex(const qbsp_t *qbsp, const int i);
u16 qbsp_light_for_vert(const qbsp_t *qbsp, const qface_t *qf, const qvec3_t v, qvec3_t sorg, qvec3_t sext, u16 *out);
