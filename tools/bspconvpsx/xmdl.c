#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/idmdl.h"
#include "../common/psxbsp.h"
#include "../common/psxmdl.h"
#include "../common/util.h"

#include "util.h"
#include "qbsp.h"
#include "qmdl.h"
#include "xbsp.h"
#include "xmdl.h"

xaliashdr_t *xbsp_entmodel_add_from_qmdl(qmdl_t *qm) {
  assert(xbsp_numentmodels < MAX_XMAP_ENTMODELS);
  assert(qm->header->numverts < MAX_XMDL_VERTS);
  assert(qm->header->numtris < MAX_XMDL_TRIS);
  assert(qm->header->numframes < MAX_XMDL_FRAMES);

  qvec3_t mins = { +1e10f, +1e10f, +1e10f };
  qvec3_t maxs = { -1e10f, -1e10f, -1e10f };
  u8 *origptr = xbsp_entmodeldataptr;
  u32 baseofs = xbsp_entmodeldataptr - xbsp_entmodeldata;
  xaliashdr_t *xmhdr = &xbsp_entmodels[xbsp_numentmodels++];
  xmhdr->type = 1; // mod_alias
  xmhdr->flags = qm->header->flags;
  xmhdr->id = qm->id;
  xmhdr->numframes = qm->header->numframes;
  xmhdr->numtris = qm->header->numtris;
  xmhdr->numverts = qm->header->numverts;
  xmhdr->scale = qvec3_to_x16vec3(qm->header->scale);
  xmhdr->offset = qvec3_to_s16vec3(qm->header->translate);
  xmhdr->trisofs = baseofs;
  xmhdr->framesofs = xmhdr->trisofs + sizeof(xaliastri_t) * xmhdr->numtris;

  // scale the texture down 2x because the skins are fucking massive
  static u8 dst[256 * 256];
  const u32 srcw = qm->header->skinwidth;
  const u32 srch = qm->header->skinheight;
  const u32 dstw = srcw >> 1;
  const u32 dsth = srch >> 1;
  const u8 *src = qm->skins[0].data;
  assert(dstw <= 256);
  assert(dsth <= 256);
  for (u32 y = 0; y < dsth; ++y) {
    for (u32 x = 0; x < dstw; ++x)
      dst[y * dstw + x] = src[(y << 1) * srcw + (x << 1)];
  }

  xtexinfo_t xti;
  int vramx = 0, vramy = 0;
  for (int i = 0; i < VRAM_NUM_PAGES; ++i) {
    if (xbsp_vram_page_fit(&xti, i, dstw >> 1, dsth, &vramx, &vramy) == 0)
      break;
  }
  xbsp_vram_store_mdltex(dst, vramx, vramy, dstw, dsth);
  xmhdr->tpage = xti.tpage;

  xaliastri_t *xmtri = (xaliastri_t *)(xbsp_entmodeldata + xmhdr->trisofs);
  for (int i = 0; i < xmhdr->numtris; ++i, ++xmtri) {
    xmtri->verts[0] = qm->tris[i].vertex[0];
    xmtri->verts[1] = qm->tris[i].vertex[1];
    xmtri->verts[2] = qm->tris[i].vertex[2];
    // TODO: normals
    for (int j = 0; j < 3; ++j) {
      const qaliastexcoord_t *qtc = &qm->texcoords[xmtri->verts[j]];
      const f32 u = ((f32)qtc->s + 0.5f) / qm->header->skinwidth;
      const f32 v = ((f32)qtc->t + 0.5f) / qm->header->skinheight;
      f32 u2 = (f32)qtc->s + (f32)qm->header->skinwidth * 0.5f;
      u2 = ((f32)u2 + 0.5f) / qm->header->skinwidth;
      xmtri->uvs[j].v = xti.uv.v + (s16)(v * dsth);
      if (qtc->onseam && !qm->tris[i].front)
        xmtri->uvs[j].u = xti.uv.u + (s16)(u2 * dstw);
      else
        xmtri->uvs[j].u = xti.uv.u + (s16)(u * dstw);
    }
  }

  xaliasvert_t *xmvert = (xaliasvert_t *)(xbsp_entmodeldata + xmhdr->framesofs);
  for (int i = 0; i < xmhdr->numframes; ++i) {
    const qaliasframe_t *qframe = &qm->frames[i];
    for (int j = 0; j < xmhdr->numverts; ++j, ++xmvert) {
      const qtrivertx_t *vert = &qframe->verts[j];
      for (int k = 0; k < 3; ++k) {
        xmvert->d[k] = vert->v[k];
        const f32 v = qm->header->scale[k] * (f32)vert->v[k] + qm->header->translate[k];
        if (v < mins[k]) mins[k] = v;
        else if (v > maxs[k]) maxs[k] = v;
      }
    }
  }

  xmhdr->mins = qvec3_to_x32vec3(mins);
  xmhdr->maxs = qvec3_to_x32vec3(maxs);

  xbsp_entmodeldataptr = (u8 *)xmvert;

  printf("* * id: %02x verts: %u, tris: %u, frames: %u size: %u skin: %ux%u extents: (%f, %f, %f)\n",
    xmhdr->id,
    xmhdr->numverts, xmhdr->numtris, xmhdr->numframes,
    (u32)(xbsp_entmodeldataptr - origptr),
    dstw, dsth,
    maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]);

  return xmhdr;
}

xaliashdr_t *xbsp_entmodel_add_from_qbsp(qbsp_t *qm, s16 id) {
  assert(xbsp_numentmodels < MAX_XMAP_ENTMODELS);

  // we only support cuboids since all of these models in OG Quake are cuboids
  assert(qm->numverts == 8);
  assert(qm->numfaces == 6);

  u8 *origptr = xbsp_entmodeldataptr;
  u32 baseofs = xbsp_entmodeldataptr - xbsp_entmodeldata;
  xaliashdr_t *xmhdr = &xbsp_entmodels[xbsp_numentmodels++];
  xmhdr->type = 1; // mod_alias
  xmhdr->flags = 0;
  xmhdr->id = id;
  xmhdr->numframes = 1;
  xmhdr->numtris = 12; // cuboid, no bottom face
  xmhdr->numverts = 8;
  xmhdr->scale = (x16vec3_t){{ ONE, ONE, ONE }}; // they all fit within 0 - 256
  xmhdr->offset = (s16vec3_t){{ 0 }};
  xmhdr->trisofs = baseofs;
  xmhdr->framesofs = xmhdr->trisofs + sizeof(xaliastri_t) * xmhdr->numtris;

  int mins[3] = { INT_MAX, INT_MAX, INT_MAX };
  int maxs[3] = { INT_MIN, INT_MIN, INT_MIN };
  for (int i = 0; i < qm->numverts; ++i) {
    for (int k = 0; k < 3; ++k) {
      const s32 v = qm->verts[i].v[k];
      if (v < mins[k]) mins[k] = v;
      else if (v > maxs[k]) maxs[k] = v;
    }
  }

  // build skin
  // stack up to 2 textures vertically
  int totalw = 0;
  int totalh = 0;
  const qmiptex_t *qmtm[2] = { NULL, NULL };
  for (int i = 0; i < qm->miptex->nummiptex && i < 2; ++i) {
    qmtm[i] = qbsp_get_miptex(qm, i);
    if (qmtm[i]->width > totalw)
      totalw = qmtm[i]->width;
    totalh += qmtm[i]->height;
  }

  // fit all textures as one contiguous block...
  xtexinfo_t xti;
  int vramx = -1, vramy = -1;
  for (int i = 0; i < VRAM_NUM_PAGES; ++i) {
    if (xbsp_vram_page_fit(&xti, i, totalw >> 1, totalh, &vramx, &vramy) == 0)
      break;
  }
  if (vramx < 0 || vramy < 0)
    panic("could not fit textures for model 0x%02x into VRAM", id);

  // ...then upload them one by one
  for (int i = 0; i < 2 && i < qm->miptex->nummiptex; ++i) {
    xbsp_vram_store_mdltex((u8 *)qmtm[i] + qmtm[i]->offsets[0], vramx, vramy, qmtm[i]->width, qmtm[i]->height);
    vramy += qmtm[i]->height;
  }

  xmhdr->tpage = xti.tpage;

  // copy verts
  xaliasvert_t *vert = (xaliasvert_t *)(xbsp_entmodeldata + xmhdr->framesofs);
  for (int i = 0; i < xmhdr->numverts; ++i) {
    vert[i].x = qm->verts[i].v[0];
    vert[i].y = qm->verts[i].v[1];
    vert[i].z = qm->verts[i].v[2];
  }

  // generate tris from faces
  xaliastri_t *tri = (xaliastri_t *)(xbsp_entmodeldata + xmhdr->trisofs);
  const qface_t *qf = qm->faces;
  for (int i = 0; i < qm->numfaces; ++i, ++qf) {
    const qtexinfo_t *qti = qm->texinfos + qf->texinfo;
    const qmiptex_t *qmt = qbsp_get_miptex(qm, qti->miptex);
    const u8 basev = qti->miptex && qmtm[1] ? qmtm[0]->height : 0;

    // get face verts and UVs
    assert(qf->numedges == 4);
    u16 qverts[4];
    f32 qst[4][2];
    f32 qstmins[2] = { 1e10f, 1e10f };
    f32 qstmaxs[2] = { -1e10f, -1e10f };
    for (int i = 0; i < qf->numedges; ++i) {
      const int e = qm->surfedges[qf->firstedge + i];
      if (e >= 0)
        qverts[i] = qm->edges[e].v[0];
      else
        qverts[i] = qm->edges[-e].v[1];
      for (int j = 0; j < 2; ++j) {
        qst[i][j] = qdot3(qm->verts[qverts[i]].v, qti->vecs[j]) + qti->vecs[j][3];
        if (qst[i][j] < qstmins[j]) qstmins[j] = qst[i][j];
        if (qst[i][j] > qstmaxs[j]) qstmaxs[j] = qst[i][j];
      }
    }

    for (int i = 0; i < qf->numedges; ++i) {
      if (qstmins[0] < 0)
        qst[i][0] += qmt->width;
      if (qstmins[1] < 0)
        qst[i][1] += qmt->height;
      qst[i][0] = (qst[i][0] / (f32)qmt->width) * (qmt->width - 1);
      qst[i][1] = (qst[i][1] / (f32)qmt->height) * (qmt->height - 1);
    }

    // triangle 1
    tri->verts[0] = qverts[0];
    tri->uvs[0].u = xti.uv.u + qst[0][0];
    tri->uvs[0].v = xti.uv.v + basev + qst[0][1];
    tri->verts[1] = qverts[1];
    tri->uvs[1].u = xti.uv.u + qst[1][0];
    tri->uvs[1].v = xti.uv.v + basev + qst[1][1];
    tri->verts[2] = qverts[2];
    tri->uvs[2].u = xti.uv.u + qst[2][0];
    tri->uvs[2].v = xti.uv.v + basev + qst[2][1];
    ++tri;

    // triangle 2
    tri->verts[0] = qverts[0];
    tri->uvs[0].u = xti.uv.u + qst[0][0];
    tri->uvs[0].v = xti.uv.v + basev + qst[0][1];
    tri->verts[1] = qverts[2];
    tri->uvs[1].u = xti.uv.u + qst[2][0];
    tri->uvs[1].v = xti.uv.v + basev + qst[2][1];
    tri->verts[2] = qverts[3];
    tri->uvs[2].u = xti.uv.u + qst[3][0];
    tri->uvs[2].v = xti.uv.v + basev + qst[3][1];
    ++tri;
  }

  xmhdr->tpage = xti.tpage;
  xmhdr->mins.x = mins[0] << FIXSHIFT;
  xmhdr->mins.y = mins[1] << FIXSHIFT;
  xmhdr->mins.z = mins[2] << FIXSHIFT;
  xmhdr->maxs.x = maxs[0] << FIXSHIFT;
  xmhdr->maxs.y = maxs[1] << FIXSHIFT;
  xmhdr->maxs.z = maxs[2] << FIXSHIFT;

  xbsp_entmodeldataptr = (u8 *)(vert + 8);

  printf("* * id: %02x verts: %u, tris: %u, frames: %u size: %u skin: %ux%u extents: (%d, %d, %d)\n",
    xmhdr->id,
    xmhdr->numverts, xmhdr->numtris, xmhdr->numframes,
    (u32)(xbsp_entmodeldataptr - origptr),
    xti.size.x, xti.size.y,
    maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]);

  return xmhdr;
}

xaliashdr_t *xbsp_entmodel_find(const s16 id) {
  for (int i = 0; i < xbsp_numentmodels; ++i) {
    if (xbsp_entmodels[i].id == id)
      return &xbsp_entmodels[i];
  }
  return NULL;
}
