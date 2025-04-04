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

void xbsp_entmodel_add_from_qmdl(qmdl_t *qm) {
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
    xmtri->normal = 0; // TODO
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
}

void xbsp_entmodel_add_from_qbsp(qbsp_t *qm, s16 id) {
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
  xmhdr->numtris = 12 - 2; // cuboid, no bottom face
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
  // TODO: merge the textures into one
  const qmiptex_t *qmt = qbsp_get_miptex(qm, 0);
  xtexinfo_t xti;
  int vramx = 0, vramy = 0;
  for (int i = 0; i < VRAM_NUM_PAGES; ++i) {
    if (xbsp_vram_page_fit(&xti, i, qmt->width >> 1, qmt->height, &vramx, &vramy) == 0)
      break;
  }
  xbsp_vram_store_mdltex((u8 *)qmt + qmt->offsets[0], vramx, vramy, qmt->width, qmt->height);
  xmhdr->tpage = xti.tpage;

  // generate cuboid from extents
  xaliasvert_t *vert = (xaliasvert_t *)(xbsp_entmodeldata + xmhdr->framesofs);
  vert[0] = (u8vec3_t){{ mins[0], mins[1], mins[2] }};
  vert[1] = (u8vec3_t){{ maxs[0], mins[1], mins[2] }};
  vert[2] = (u8vec3_t){{ mins[0], maxs[1], mins[2] }};
  vert[3] = (u8vec3_t){{ maxs[0], maxs[1], mins[2] }};
  vert[4] = (u8vec3_t){{ maxs[0], mins[1], maxs[2] }};
  vert[5] = (u8vec3_t){{ mins[0], mins[1], maxs[2] }};
  vert[6] = (u8vec3_t){{ maxs[0], maxs[1], maxs[2] }};
  vert[7] = (u8vec3_t){{ mins[0], maxs[1], maxs[2] }};

  // generate indices
  xaliastri_t *tri = (xaliastri_t *)(xbsp_entmodeldata + xmhdr->trisofs);
  tri[0].verts[0] = 4; tri[0].verts[1] = 5; tri[0].verts[2] = 6;
  tri[1].verts[0] = 7; tri[1].verts[1] = 6; tri[1].verts[2] = 5;
  tri[2].verts[0] = 5; tri[2].verts[1] = 4; tri[2].verts[2] = 0;
  tri[3].verts[0] = 1; tri[3].verts[1] = 0; tri[3].verts[2] = 4;
  tri[4].verts[0] = 6; tri[4].verts[1] = 7; tri[4].verts[2] = 3;
  tri[5].verts[0] = 2; tri[5].verts[1] = 3; tri[5].verts[2] = 7;
  tri[6].verts[0] = 0; tri[6].verts[1] = 2; tri[6].verts[2] = 5;
  tri[7].verts[0] = 7; tri[7].verts[1] = 5; tri[7].verts[2] = 2;
  tri[8].verts[0] = 3; tri[8].verts[1] = 1; tri[8].verts[2] = 6;
  tri[9].verts[0] = 4; tri[9].verts[1] = 6; tri[9].verts[2] = 1;

  // generate UVs
  const u8 uvw = qmt->width - 1;
  const u8 uvh = qmt->height - 1;
  const u8 uvx = xti.uv.u;
  const u8 uvy = xti.uv.v;
  for (int i = 0; i < 10; i += 2) {
    tri[i + 0].uvs[0] = (u8vec2_t){{ uvx +   0, uvy +   0 }};
    tri[i + 0].uvs[1] = (u8vec2_t){{ uvx + uvw, uvy +   0 }};
    tri[i + 0].uvs[2] = (u8vec2_t){{ uvx +   0, uvy + uvh }};
    tri[i + 1].uvs[2] = (u8vec2_t){{ uvx + uvw, uvy +   0 }};
    tri[i + 1].uvs[1] = (u8vec2_t){{ uvx +   0, uvy + uvh }};
    tri[i + 1].uvs[0] = (u8vec2_t){{ uvx + uvw, uvy + uvh }};
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
}

xaliashdr_t *xbsp_entmodel_find(const s16 id) {
  for (int i = 0; i < xbsp_numentmodels; ++i) {
    if (xbsp_entmodels[i].id == id)
      return &xbsp_entmodels[i];
  }
  return NULL;
}
