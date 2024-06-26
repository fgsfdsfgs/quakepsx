#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include <inline_n.h>

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
  u8vec2_t tex;
  u8vec4_t col;
  s32 sz;
} __attribute__((packed)) svert_t;

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
  h01->tex.u = sv0->tex.u + ((sv1->tex.u - sv0->tex.u) >> 1);
  h01->tex.v = sv0->tex.v + ((sv1->tex.v - sv0->tex.v) >> 1);
  h01->col.r = sv0->col.r + ((sv1->col.r - sv0->col.r) >> 1);
  h01->col.g = sv0->col.g + ((sv1->col.g - sv0->col.g) >> 1);
  h01->col.b = sv0->col.b + ((sv1->col.b - sv0->col.b) >> 1);
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
    *(u32 *)&poly->x0 = *(u32 *)&pv0->pos.x; \
    *(u32 *)&poly->x1 = *(u32 *)&pv1->pos.x; \
    *(u32 *)&poly->x2 = *(u32 *)&pv2->pos.x; \
    poly = EmitBrushTriangle(poly, tpage, otz, cv0, cv1, cv2); \
  }

#define SORT_FTRI(pv0, pv1, pv2, cv0, cv1, cv2, az) \
  *(u32 *)&poly->x0 = *(u32 *)&pv0->pos.x; \
  *(u32 *)&poly->x1 = *(u32 *)&pv1->pos.x; \
  *(u32 *)&poly->x2 = *(u32 *)&pv2->pos.x; \
  poly = EmitBrushTriangle(poly, tpage, az, cv0, cv1, cv2);

#define SORT_QUAD(pv0, pv1, pv2, pv3, cv0, cv1, cv2, cv3) \
  gte_ldsz4(pv0->sz, pv1->sz, pv2->sz, pv3->sz); \
  gte_avsz4(); \
  gte_stotz_m(otz); \
  if (otz > 0) { \
    *(u32 *)&((POLY_GT4 *)poly)->x0 = *(u32 *)&pv0->pos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x1 = *(u32 *)&pv1->pos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x2 = *(u32 *)&pv2->pos.x; \
    *(u32 *)&((POLY_GT4 *)poly)->x3 = *(u32 *)&pv3->pos.x; \
    poly = EmitBrushQuad((void *)poly, tpage, otz, cv0, cv1, cv2, cv3); \
  }

static inline POLY_GT3 *SubdivBrushTriangle1(POLY_GT3 *poly, const u16 tpage, s32 botz, svert_t *sv0, svert_t *sv1, svert_t *sv2, svert_t *clipzone)
{
  register int otz;

  // calculate halfpoints on every edge
  svert_t *h01 = HalfwayVert(clipzone + 3, sv0, sv1);
  svert_t *h12 = HalfwayVert(clipzone + 4, sv1, sv2);
  svert_t *h20 = HalfwayVert(clipzone + 5, sv2, sv0);

  // store the original transformed verts
  svert_t *tv0 = clipzone + 0;
  svert_t *tv1 = clipzone + 1;
  svert_t *tv2 = clipzone + 2;
  gte_stsxy3(&tv0->pos.x, &tv1->pos.x, &tv2->pos.x);
  gte_stsz3(&tv0->sz, &tv1->sz, &tv2->sz);

  // transform the new verts and save the result
  gte_ldv3(&h01->pos.x, &h12->pos.x, &h20->pos.x);
  gte_rtpt();
  gte_stsxy3(&h01->pos.x, &h12->pos.x, &h20->pos.x);
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

  // store the original transformed verts
  svert_t *tv00 = clipzone + 0;
  svert_t *tv01 = clipzone + 1;
  svert_t *tv02 = clipzone + 2;
  gte_stsxy3(&tv00->pos.x, &tv01->pos.x, &tv02->pos.x);
  gte_stsz3(&tv00->sz, &tv01->sz, &tv02->sz);

  // calculate halfpoints on every edge and halfpoints on every half-edge
  svert_t *tv03 = HalfwayVert(clipzone +  3, sv00, sv01);
  svert_t *tv04 = HalfwayVert(clipzone +  4, sv01, sv02);
  svert_t *tv05 = HalfwayVert(clipzone +  5, sv00, sv02);
  svert_t *tv06 = HalfwayVert(clipzone +  6, sv00, tv03);
  svert_t *tv07 = HalfwayVert(clipzone +  7, sv01, tv03);
  svert_t *tv08 = HalfwayVert(clipzone +  8, sv01, tv04);
  svert_t *tv09 = HalfwayVert(clipzone +  9, tv04, sv02);
  svert_t *tv10 = HalfwayVert(clipzone + 10, tv05, sv02);
  svert_t *tv11 = HalfwayVert(clipzone + 11, tv05, sv00);
  svert_t *tv12 = HalfwayVert(clipzone + 12, tv05, tv03);
  svert_t *tv13 = HalfwayVert(clipzone + 13, tv12, tv08);
  svert_t *tv14 = HalfwayVert(clipzone + 14, tv05, tv04);

  // transform them
  gte_ldv3(&tv03->pos.x, &tv04->pos.x, &tv05->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv03->pos.x, &tv04->pos.x, &tv05->pos.x);
  gte_stsz3(&tv03->sz, &tv04->sz, &tv05->sz);
  gte_ldv3(&tv06->pos.x, &tv07->pos.x, &tv08->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv06->pos.x, &tv07->pos.x, &tv08->pos.x);
  gte_stsz3(&tv06->sz, &tv07->sz, &tv08->sz);
  gte_ldv3(&tv09->pos.x, &tv10->pos.x, &tv11->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv09->pos.x, &tv10->pos.x, &tv11->pos.x);
  gte_stsz3(&tv09->sz, &tv10->sz, &tv11->sz);
  gte_ldv3(&tv12->pos.x, &tv13->pos.x, &tv14->pos.x);
  gte_rtpt();
  gte_stsxy3(&tv12->pos.x, &tv13->pos.x, &tv14->pos.x);
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

  // transform the big triangle
  gte_ldv3(&sv0->pos.x, &sv1->pos.x, &sv2->pos.x);
  gte_rtpt();
  gte_stsxy3_gt3(poly);

  // calculate OT Z for the big triangle
  gte_avsz3();
  gte_stotz_m(otz);

  // clip off if out of Z range
  if (otz <= 0 || otz >= GPU_OTDEPTH)
    return poly;

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

  // put all the verts into scratch and light them; this will also ensure that pos is aligned to 4
  // that means we're limited to ~50 verts per poly, since we use the scratch to store clipped verts as well
  for (int i = 0; i < numverts; ++i, ++v, ++sv) {
    sv->pos = v->pos;
    sv->tex.word = v->tex.word;
    sv->col.word = LightVert(v->col, fa->styles);
  }

  // reset pointers
  clipzone = sv;
  sv = (svert_t *)PSX_SCRATCH;

  // it's a triangle fan
  for (int i = 2; i < numverts; ++i) {
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
