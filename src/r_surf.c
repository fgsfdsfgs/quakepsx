#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

#include "common.h"
#include "system.h"
#include "render.h"
#include "entity.h"
#include "game.h"

#define CLIP_LEFT 1
#define CLIP_RIGHT 2
#define CLIP_TOP 4
#define CLIP_BOTTOM 8

typedef struct {
  s16vec3_t pos;
  s16 pad;
} __attribute__((packed)) savert_t;

typedef struct {
  s16vec3_t pos;
  u8vec2_t tex;
  u8vec4_t col;
  s16vec2_t tpos;
  s32 sz;
} __attribute__((packed)) svert_t;

static savert_t alias_verts[256];

static inline s16 TestClip(const RECT *clip, const s16 x, const s16 y) {
  // tests which corners of the screen a point lies outside of
  register s16 result = 0;
  if (x < clip->x)
    result |= CLIP_LEFT;
  if (x >= (clip->x+(clip->w-1)))
    result |= CLIP_RIGHT;
  if (y < clip->y)
    result |= CLIP_TOP;
  if (y >= (clip->y+(clip->h-1)))
    result |= CLIP_BOTTOM;
  return result;
}

static inline qboolean TriClip(const RECT *clip, const POLY_GT3 *poly) {
  // returns non-zero if a triangle is outside the screen boundaries
  register const s16 c0 = TestClip(clip, poly->x0, poly->y0);
  register const s16 c1 = TestClip(clip, poly->x1, poly->y1);
  register const s16 c2 = TestClip(clip, poly->x2, poly->y2);

  if ((c0 & c1) == 0)
    return false;
  if ((c1 & c2) == 0)
    return false;
  if ((c2 & c0) == 0)
    return false;

  return true;
}

static inline qboolean QuadClip(const RECT *clip, const POLY_GT4 *poly) {
  // returns non-zero if a quad is outside the screen boundaries
  register const s16 c0 = TestClip(clip, poly->x0, poly->y0);
  register const s16 c1 = TestClip(clip, poly->x1, poly->y1);
  register const s16 c2 = TestClip(clip, poly->x2, poly->y2);
  register const s16 c3 = TestClip(clip, poly->x3, poly->y3);

  if ((c0 & c1) == 0)
    return false;
  if ((c1 & c2) == 0)
    return false;
  if ((c2 & c3) == 0)
    return false;
  if ((c3 & c0) == 0)
    return false;
  if ((c0 & c2) == 0)
    return false;
  if ((c1 & c3) == 0)
    return false;

  return true;
}

static inline u32 LightVert(const u8 *col, const u8 *styles) {
  register u32 lit;
  lit  = col[0] * r_lightstylevalue[styles[0]];
  lit += col[1] * r_lightstylevalue[styles[1]];
  lit  = (lit >> 8);
  return lit | (lit << 8) | (lit << 16);
}

static inline void *EmitAliasTriangle(POLY_GT3 *poly, const u16 tpage, const int otz, const savert_t *sv0, const savert_t *sv1, const savert_t *sv2)
{
  if (!TriClip(&rs.clip, poly)) {
    setPolyGT3(poly);
    poly->tpage = tpage;
    poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
    addPrim(gpu_ot + otz, poly);
    ++c_draw_polys;
    ++poly;
  }
  return poly;
}

static inline void *EmitBrushTriangle(POLY_GT3 *poly, const u16 tpage, const int otz, const svert_t *sv0, const svert_t *sv1, const svert_t *sv2)
{
  if (!TriClip(&rs.clip, poly)) {
    *(u32 *)&poly->r0 = sv0->col.word;
    *(u32 *)&poly->r1 = sv1->col.word;
    *(u32 *)&poly->r2 = sv2->col.word;
    *(u16 *)&poly->u0 = sv0->tex.word;
    *(u16 *)&poly->u1 = sv1->tex.word;
    *(u16 *)&poly->u2 = sv2->tex.word;
    setPolyGT3(poly);
    poly->tpage = tpage;
    poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
    addPrim(gpu_ot + otz, poly);
    ++c_draw_polys;
    ++poly;
  }
  return poly;
}

static inline void *EmitBrushQuad(POLY_GT4 *poly, const u16 tpage, const s32 otz, const svert_t *sv0, const svert_t *sv1, const svert_t *sv2 , const svert_t *sv3)
{
  if (!QuadClip(&rs.clip, poly)) {
    *(u32 *)&poly->r0 = sv0->col.word;
    *(u32 *)&poly->r1 = sv1->col.word;
    *(u32 *)&poly->r2 = sv2->col.word;
    *(u32 *)&poly->r3 = sv3->col.word;
    *(u16 *)&poly->u0 = sv0->tex.word;
    *(u16 *)&poly->u1 = sv1->tex.word;
    *(u16 *)&poly->u2 = sv2->tex.word;
    *(u16 *)&poly->u3 = sv3->tex.word;
    setPolyGT4(poly);
    poly->tpage = tpage;
    poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
    addPrim(gpu_ot + otz, poly);
    c_draw_polys += 2;
    ++poly;
  }
  return poly;
}

static inline svert_t *HalfwayVert(svert_t *h01, const svert_t *sv0, const svert_t *sv1)
{
  h01->tex.u = (s16)sv0->tex.u + (((s16)sv1->tex.u - (s16)sv0->tex.u) >> 1);
  h01->tex.v = (s16)sv0->tex.v + (((s16)sv1->tex.v - (s16)sv0->tex.v) >> 1);
  h01->col.r = (s16)sv0->col.r + (((s16)sv1->col.r - (s16)sv0->col.r) >> 1);
  h01->col.g = h01->col.r; // assume non-colored lighting for now
  h01->col.b = h01->col.r;
  h01->pos.x = sv0->pos.x + ((sv1->pos.x - sv0->pos.x) >> 1);
  h01->pos.y = sv0->pos.y + ((sv1->pos.y - sv0->pos.y) >> 1);
  h01->pos.z = sv0->pos.z + ((sv1->pos.z - sv0->pos.z) >> 1);
  return h01;
}

#define SORT_TRI(pv0, pv1, pv2, cv0, cv1, cv2) \
  gte_ldsz3(pv0->sz, pv1->sz, pv2->sz); \
  gte_avsz3(); \
  gte_stotz_m(otz); \
  if (otz > 0) { \
    *(u32 *)&poly->x0 = *(u32 *)&pv0->tpos.x; \
    *(u32 *)&poly->x1 = *(u32 *)&pv1->tpos.x; \
    *(u32 *)&poly->x2 = *(u32 *)&pv2->tpos.x; \
    poly = EmitBrushTriangle(poly, tpage, otz, cv0, cv1, cv2); \
  }

#define SORT_FTRI(pv0, pv1, pv2, cv0, cv1, cv2, az) \
  *(u32 *)&poly->x0 = *(u32 *)&pv0->tpos.x; \
  *(u32 *)&poly->x1 = *(u32 *)&pv1->tpos.x; \
  *(u32 *)&poly->x2 = *(u32 *)&pv2->tpos.x; \
  poly = EmitBrushTriangle(poly, tpage, az, cv0, cv1, cv2);

#define SORT_QUAD(pv0, pv1, pv2, pv3, cv0, cv1, cv2, cv3) \
  gte_ldsz4(pv0->sz, pv1->sz, pv2->sz, pv3->sz); \
  gte_avsz4(); \
  gte_stotz_m(otz); \
  if (otz > 0) { \
    *(u32 *)&((POLY_GT4 *)poly)->x0 = *(u32 *)&pv0->tpos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x1 = *(u32 *)&pv1->tpos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x2 = *(u32 *)&pv2->tpos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x3 = *(u32 *)&pv3->tpos.x; \
    poly = EmitBrushQuad((void *)poly, tpage, otz, cv0, cv1, cv2, cv3); \
  }

static inline POLY_GT3 *SubdivBrushTriangle1(POLY_GT3 *poly, const u16 tpage, s32 botz, svert_t *sv0, svert_t *sv1, svert_t *sv2, svert_t *clipzone)
{
  register int otz;

  // calculate halfpoints on every edge
  svert_t *h01 = HalfwayVert(clipzone + 0, sv0, sv1);
  svert_t *h12 = HalfwayVert(clipzone + 1, sv1, sv2);
  svert_t *h20 = HalfwayVert(clipzone + 2, sv2, sv0);

  // the original transformed verts
  svert_t *tv0 = sv0;
  svert_t *tv1 = sv1;
  svert_t *tv2 = sv2;

  // transform the new verts and save the result
  gte_ldv3(&h01->pos.x, &h12->pos.x, &h20->pos.x);
  gte_rtpt();
  gte_stsxy3(&h01->tpos.x, &h12->tpos.x, &h20->tpos.x);
  gte_stsz3(&h01->sz, &h12->sz, &h20->sz);

  // sort the resulting 4 triangles as 1 quad + 2 tris
  // sv0 - h01 - h20 - h12
  SORT_QUAD(tv0, h01, h20, h12, sv0, h01, h20, h12);
  // h01 - sv1 - h12
  SORT_TRI(h01, tv1, h12, h01, sv1, h12);
  // h12 - sv2 - h20
  SORT_TRI(h12, tv2, h20, h12, sv2, h20);

  // fill the holes between new triangles if the original triangle goes far enough
  otz = tv0->sz;
  if (tv1->sz > otz) otz = tv1->sz;
  if (tv2->sz > otz) otz = tv2->sz;
  if (otz >= GPU_SUBDIV_DIST_1) {
    botz += 8;
    SORT_FTRI(tv0, tv1, h01, sv0, sv1, h01, botz);
    SORT_FTRI(tv1, tv2, h12, sv0, sv1, h12, botz);
    SORT_FTRI(tv2, tv0, h20, sv2, sv0, h20, botz);
  }

  return poly;
}

static inline POLY_GT3 *SubdivBrushTriangle2(POLY_GT3 *poly, const u16 tpage, s32 botz, svert_t *sv00, svert_t *sv01, svert_t *sv02, svert_t *clipzone)
{
  register int otz;

  // the original transformed verts
  svert_t *tv00 = sv00;
  svert_t *tv01 = sv01;
  svert_t *tv02 = sv02;

  // calculate halfpoints on every edge and halfpoints on every half-edge
  svert_t *tv03 = HalfwayVert(clipzone +  0, sv00, sv01);
  svert_t *tv04 = HalfwayVert(clipzone +  1, sv01, sv02);
  svert_t *tv05 = HalfwayVert(clipzone +  2, sv00, sv02);
  svert_t *tv06 = HalfwayVert(clipzone +  3, sv00, tv03);
  svert_t *tv07 = HalfwayVert(clipzone +  4, sv01, tv03);
  svert_t *tv08 = HalfwayVert(clipzone +  5, sv01, tv04);
  svert_t *tv09 = HalfwayVert(clipzone +  6, tv04, sv02);
  svert_t *tv10 = HalfwayVert(clipzone +  7, tv05, sv02);
  svert_t *tv11 = HalfwayVert(clipzone +  8, tv05, sv00);
  svert_t *tv12 = HalfwayVert(clipzone +  9, tv05, tv03);
  svert_t *tv13 = HalfwayVert(clipzone + 10, tv12, tv08);
  svert_t *tv14 = HalfwayVert(clipzone + 11, tv05, tv04);

  // transform them
  gte_ldv3(&tv03->pos.x, &tv04->pos.x, &tv05->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv03->tpos.x, &tv04->tpos.x, &tv05->tpos.x);
  gte_stsz3(&tv03->sz, &tv04->sz, &tv05->sz);
  gte_ldv3(&tv06->pos.x, &tv07->pos.x, &tv08->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv06->tpos.x, &tv07->tpos.x, &tv08->tpos.x);
  gte_stsz3(&tv06->sz, &tv07->sz, &tv08->sz);
  gte_ldv3(&tv09->pos.x, &tv10->pos.x, &tv11->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv09->tpos.x, &tv10->tpos.x, &tv11->tpos.x);
  gte_stsz3(&tv09->sz, &tv10->sz, &tv11->sz);
  gte_ldv3(&tv12->pos.x, &tv13->pos.x, &tv14->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv12->tpos.x, &tv13->tpos.x, &tv14->tpos.x);
  gte_stsz3(&tv12->sz, &tv13->sz, &tv14->sz);

  // corner triangles
  SORT_TRI(tv00, tv06, tv11, sv00, tv06, tv11);
  SORT_TRI(tv11, tv12, tv05, tv11, tv12, tv05);
  SORT_TRI(tv05, tv14, tv10, tv05, tv14, tv10);
  SORT_TRI(tv10, tv09, tv02, tv10, tv09, sv02);
  // the rest is quads
  SORT_QUAD(tv06, tv03, tv11, tv12, tv06, tv03, tv11, tv12);
  SORT_QUAD(tv03, tv07, tv12, tv13, tv03, tv07, tv12, tv13);
  SORT_QUAD(tv07, tv01, tv13, tv08, tv07, sv01, tv13, tv08);
  SORT_QUAD(tv12, tv13, tv05, tv14, tv12, tv13, tv05, tv14);
  SORT_QUAD(tv13, tv08, tv14, tv04, tv13, tv08, tv14, tv04);
  SORT_QUAD(tv14, tv04, tv10, tv09, tv14, tv04, tv10, tv09);

  // fill the holes between new triangles if the original triangle goes far enough
  otz = tv00->sz;
  if (tv01->sz > otz) otz = tv01->sz;
  if (tv02->sz > otz) otz = tv02->sz;
  if (otz >= GPU_SUBDIV_DIST_2) {
    botz += 8;
    SORT_FTRI(tv00, tv01, tv03, sv00, sv01, tv03, botz);
    SORT_FTRI(tv00, tv03, tv06, sv00, tv03, tv06, botz);
    SORT_FTRI(tv03, tv01, tv07, tv03, sv01, tv07, botz);
    SORT_FTRI(tv01, tv02, tv04, sv01, sv02, tv04, botz);
    SORT_FTRI(tv01, tv04, tv08, sv01, tv04, tv08, botz);
    SORT_FTRI(tv04, tv02, tv09, tv04, sv02, tv09, botz);
    SORT_FTRI(tv00, tv02, tv05, sv00, sv02, tv05, botz);
    SORT_FTRI(tv00, tv05, tv11, sv00, tv05, tv11, botz);
    SORT_FTRI(tv05, tv02, tv10, tv05, sv02, tv10, botz);
  }

  return poly;
}

#undef SORT_TRI
#undef SORT_FTRI
#undef SORT_QUAD

static inline POLY_GT3 *RenderBrushTriangle(POLY_GT3 *poly, const u16 tpage, svert_t *sv0, svert_t *sv1, svert_t *sv2, svert_t *clipzone)
{
  register int otz;

  // calculate OT Z for the big triangle
  gte_ldsz3(sv0->sz, sv1->sz, sv2->sz);
  gte_avsz3();
  gte_stotz_m(otz);

  // clip off if out of Z range
  if (otz <= 0 || otz >= GPU_OTDEPTH)
    return poly;

  // fill in the transformed coords
  *(u32 *)&poly->x0 = *(u32 *)&sv0->tpos.x;
  *(u32 *)&poly->x1 = *(u32 *)&sv1->tpos.x;
  *(u32 *)&poly->x2 = *(u32 *)&sv2->tpos.x;

  // see if we need to subdivide
  if (otz < GPU_SUBDIV_DIST_2) {
    // subdivide twice
    poly = SubdivBrushTriangle2(poly, tpage, otz, sv0, sv1, sv2, clipzone);
  } else if (otz < GPU_SUBDIV_DIST_1) {
    // subdivide once
    poly = SubdivBrushTriangle1(poly, tpage, otz, sv0, sv1, sv2, clipzone);
  } else {
    // do not need to subdivide, emit as is
    poly = EmitBrushTriangle(poly, tpage, otz, sv0, sv1, sv2);
  }

  return poly;
}

static inline void RenderBrushPoly(const msurface_t *fa) {
  const mvert_t *v = gs.worldmodel->verts + fa->firstvert;
  const u16 tpage = fa->texture->vram_page;
  const int numverts = fa->numverts;
  svert_t *sv = PSX_SCRATCH;
  svert_t *clipzone;
  POLY_GT3 *poly = (POLY_GT3 *)gpu_ptr;
  int i;

  // put all the verts into scratch and light them; this will also ensure that pos is aligned to 4
  // that means we're limited to ~50 verts per poly, since we use the scratch to store clipped verts as well
  for (i = 0; i < numverts; ++i, ++v, ++sv) {
    sv->pos = v->pos;
    sv->tex.word = v->tex.word;
    sv->col.word = LightVert(v->col, fa->styles);
  }

  // clipverts go after the normal verts
  clipzone = sv;

  // transform all verts in threes
  sv = (svert_t *)PSX_SCRATCH;
  for (i = 0; i <= numverts - 3; i += 3, sv += 3) {
    gte_ldv3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_rtpt();
    gte_stsxy3(&sv[0].tpos.x, &sv[1].tpos.x, &sv[2].tpos.x);
    gte_stsz3(&sv[0].sz, &sv[1].sz, &sv[2].sz);
  }
  // transform the remaining 1-2 verts with rtps
  for (; i < numverts; ++i, ++sv) {
    gte_ldv0(&sv->pos.x);
    gte_rtps();
    gte_stsxy(&sv->tpos.x);
    gte_stsz(&sv->sz);
  }

  // it's a triangle fan
  sv = (svert_t *)PSX_SCRATCH;
  for (i = 2; i < numverts; ++i) {
    poly = RenderBrushTriangle(poly, tpage, &sv[0], &sv[i - 1], &sv[i], clipzone);
  }

  gpu_ptr = (void *)poly;
}

void R_DrawTextureChains(void) {
  for (int i = 0; i < gs.worldmodel->numtextures; ++i) {
    mtexture_t *t = gs.worldmodel->textures + i;
    if (!t || (t->flags & TEX_INVISIBLE))
      continue;
    msurface_t *s = t->texchain;
    if (!s) continue;
    if (t->flags & TEX_SKY) {
      // draw sky
    } else {
      for (; s; s = s->texchain)
        RenderBrushPoly(s);
    }
    t->texchain = NULL;
  }
}

void R_DrawAliasModel(amodel_t *model, int frame) {
  register int otz;
  const u32 col = 0x808080; // TODO: calculate light level
  const u16 tpage = model->tpage;
  const int numverts = model->numverts;
  const int numtris = model->numtris;
  const u8vec3_t *averts = model->frames + (frame * numverts);
  POLY_GT3 *poly = (POLY_GT3 *)gpu_ptr;
  int i;

  // transform all verts first
  const u8vec3_t *av = averts;
  savert_t *sv = alias_verts;
  for (i = 0; i <= numverts - 3; i += 3, av += 3, sv += 3) {
    sv[0].pos.x = (av[0].x * model->scale.x) >> FIXSHIFT;
    sv[0].pos.y = (av[0].y * model->scale.y) >> FIXSHIFT;
    sv[0].pos.z = (av[0].z * model->scale.z) >> FIXSHIFT;
    sv[1].pos.x = (av[1].x * model->scale.x) >> FIXSHIFT;
    sv[1].pos.y = (av[1].y * model->scale.y) >> FIXSHIFT;
    sv[1].pos.z = (av[1].z * model->scale.z) >> FIXSHIFT;
    sv[2].pos.x = (av[2].x * model->scale.x) >> FIXSHIFT;
    sv[2].pos.y = (av[2].y * model->scale.y) >> FIXSHIFT;
    sv[2].pos.z = (av[2].z * model->scale.z) >> FIXSHIFT;
    gte_ldv3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_rtpt();
    gte_stsxy3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_stsz3(&sv[0].pos.z, &sv[1].pos.z, &sv[2].pos.z);
  }
  // transform the remaining 1-2 verts
  for (; i < numverts; ++i, ++sv, ++av) {
    sv[0].pos.x = (av[0].x * model->scale.x) >> FIXSHIFT;
    sv[0].pos.y = (av[0].y * model->scale.y) >> FIXSHIFT;
    sv[0].pos.z = (av[0].z * model->scale.z) >> FIXSHIFT;
    gte_ldv0(&sv[0].pos.x);
    gte_rtps();
    gte_stsxy(&sv[0].pos.x);
    gte_stsz(&sv[0].pos.z);
  }

  const atri_t *tri = model->tris;
  for (int i = 0; i < numtris; ++i, ++tri) {
    const qboolean back = tri->fnorm & 0x80;
    const savert_t *sv0 = &alias_verts[tri->verts[0]];
    const savert_t *sv1 = &alias_verts[tri->verts[1]];
    const savert_t *sv2 = &alias_verts[tri->verts[2]];
    const u8vec3_t *uv0 = &model->texcoords[tri->verts[0]];
    const u8vec3_t *uv1 = &model->texcoords[tri->verts[1]];
    const u8vec3_t *uv2 = &model->texcoords[tri->verts[2]];

    // cull backfaces
    gte_ldsxy3(&sv0->pos.x, &sv1->pos.x, &sv2->pos.x);
    gte_nclip();
    gte_stopz_m(otz);
    if (otz < 0)
      continue;

    // calculate OT Z for the big triangle
    gte_ldsz3(sv0->pos.z, sv1->pos.z, sv2->pos.z);
    gte_avsz3();
    gte_stotz_m(otz);
    // clip off if out of Z range
    if (otz <= 0 || otz >= GPU_OTDEPTH)
      continue;

    // set positions, UVs and colors
    *(u32 *)&poly->x0 = *(u32 *)&sv0->pos.x;
    *(u32 *)&poly->x1 = *(u32 *)&sv1->pos.x;
    *(u32 *)&poly->x2 = *(u32 *)&sv2->pos.x;
    if (back) {
      poly->u0 = uv0->p;
      poly->u1 = uv1->p;
      poly->u2 = uv2->p;
    } else {
      poly->u0 = uv0->u;
      poly->u1 = uv1->u;
      poly->u2 = uv2->u;
    }
    poly->v0 = uv0->v;
    poly->v1 = uv1->v;
    poly->v2 = uv2->v;
    *(u32 *)&poly->r0 = col;
    *(u32 *)&poly->r1 = col;
    *(u32 *)&poly->r2 = col;

    poly = EmitAliasTriangle(poly, tpage, otz, sv0, sv1, sv2);
  }

  gpu_ptr = (u8 *)poly;
}

void R_DrawBrushModel(bmodel_t *model) {
  msurface_t *psurf;
  mplane_t *pplane;
  x32 dot;

  psurf = &model->surfaces[model->firstmodelsurface];

  for (int i = 0; i < model->nummodelsurfaces; ++i, ++psurf) {
    // find which side of the node we are on
    pplane = psurf->plane;
    dot = XVecDotSL(&pplane->normal, &rs.vieworg) - pplane->dist;
    // draw the polygon
    if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
      (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON))) {
      psurf->texchain = psurf->texture->texchain;
      psurf->texture->texchain = psurf;
    }
  }
}

static inline LINE_F3 *DrawTwoLines(LINE_F3 *line, const int otz, const u32 col, const SVECTOR *va, const SVECTOR *vb, const SVECTOR *vc)
{
  if (va->vx < 0 || va->vx > rs.clip.w || va->vy < 0 || va->vy > rs.clip.h)
    return line;
  if (vb->vx < 0 || vb->vx > rs.clip.w || vb->vy < 0 || vb->vy > rs.clip.h)
    return line;
  if (vc->vx < 0 || vc->vx > rs.clip.w || vc->vy < 0 || vc->vy > rs.clip.h)
    return line;
  if (otz <= 0 || otz >= GPU_OTDEPTH)
    return line;

  line->x0 = va->vx;
  line->y0 = va->vy;
  line->x1 = vb->vx;
  line->y1 = vb->vy;
  line->x2 = vc->vx;
  line->y2 = vc->vy;
  *(u32 *)&line->r0 = col;

  setLineF3(line);
  addPrim(gpu_ot + otz, line);
  ++c_draw_polys;
  ++line;
  return line;
}

void R_DrawBBox(edict_t *ent)
{
  register int otz1, otz2;

  SVECTOR *svp = PSX_SCRATCH;
  SVECTOR *svmin = svp;
  svp->vx = ent->v.absmin.x >> FIXSHIFT;
  svp->vy = ent->v.absmin.y >> FIXSHIFT;
  svp->vz = ent->v.absmin.z >> FIXSHIFT;
  svp++;
  SVECTOR *svmax = svp;
  svp->vx = ent->v.absmax.x >> FIXSHIFT;
  svp->vy = ent->v.absmax.y >> FIXSHIFT;
  svp->vz = ent->v.absmax.z >> FIXSHIFT;
  svp++;
  SVECTOR *sv00 = svmin;
  SVECTOR *sv10 = svp;
  svp->vx = svmax->vx;
  svp->vy = svmin->vy;
  svp->vz = svmin->vz;
  svp++;
  SVECTOR *sv20 = svp;
  svp->vx = svmax->vx;
  svp->vy = svmax->vy;
  svp->vz = svmin->vz;
  svp++;
  SVECTOR *sv30 = svp;
  svp->vx = svmin->vx;
  svp->vy = svmax->vy;
  svp->vz = svmin->vz;
  svp++;
  SVECTOR *sv01 = svp;
  svp->vx = svmin->vx;
  svp->vy = svmin->vy;
  svp->vz = svmax->vz;
  svp++;
  SVECTOR *sv11 = svp;
  svp->vx = svmax->vx;
  svp->vy = svmin->vy;
  svp->vz = svmax->vz;
  svp++;
  SVECTOR *sv21 = svmax;
  SVECTOR *sv31 = svp;
  svp->vx = svmin->vx;
  svp->vy = svmax->vy;
  svp->vz = svmax->vz;
  svp++;

  SVECTOR *tv00 = svp++;
  SVECTOR *tv10 = svp++;
  SVECTOR *tv20 = svp++;
  SVECTOR *tv30 = svp++;
  SVECTOR *tv01 = svp++;
  SVECTOR *tv11 = svp++;
  SVECTOR *tv21 = svp++;
  SVECTOR *tv31 = svp++;

  gte_ldv3(sv00, sv10, sv20);
  gte_rtpt();
  gte_stsxy3(tv00, tv10, tv20);
  gte_ldv0(sv30);
  gte_rtps();
  gte_stsxy(tv30);
  gte_avsz4();
  gte_stotz_m(otz1);

  gte_ldv3(sv01, sv11, sv21);
  gte_rtpt();
  gte_stsxy3(tv01, tv11, tv21);
  gte_ldv0(sv31);
  gte_rtps();
  gte_stsxy(tv31);
  gte_avsz4();
  gte_stotz_m(otz2);

  LINE_F3 *line = (LINE_F3 *)gpu_ptr;

  line = DrawTwoLines(line, otz1, 0x008000, tv00, tv10, tv20);
  line = DrawTwoLines(line, otz1, 0x008000, tv20, tv30, tv00);
  line = DrawTwoLines(line, otz2, 0x008000, tv01, tv11, tv21);
  line = DrawTwoLines(line, otz2, 0x008000, tv21, tv31, tv01);

  gpu_ptr = (u8 *)line;
}
