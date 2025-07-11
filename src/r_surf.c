#include <stdlib.h>
#include <sys/types.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

#include "common.h"
#include "render.h"
#include "entity.h"
#include "game.h"

#define CLIP_LEFT 1
#define CLIP_RIGHT 2
#define CLIP_TOP 4
#define CLIP_BOTTOM 8

#define PT_SIZE_SHIFT 8
#define PT_SIZE_MIN 1
#define PT_SIZE_MAX 4

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

typedef struct {
  s16vec3_t pos;
  s16 pad;
  u8vec4_t col;
  s32 sz;
} __attribute__((packed)) sbvert_t;

static savert_t alias_verts[MAX_XMDL_VERTS];

static inline u8 TestClip(const s16 x, const s16 y) {
  // tests which corners of the screen a point lies outside of
  register u8 result = 0;
  if (x < 0)
    result |= CLIP_LEFT;
  if (x >= (VID_WIDTH - 1))
    result |= CLIP_RIGHT;
  if (y < 0)
    result |= CLIP_TOP;
  if (y >= (VID_HEIGHT - 1))
    result |= CLIP_BOTTOM;
  return result;
}

static inline qboolean TriClip(const POLY_GT3 *poly) {
  // returns non-zero if a triangle is outside the screen boundaries
  register const u8 c0 = TestClip(poly->x0, poly->y0);
  register const u8 c1 = TestClip(poly->x1, poly->y1);
  register const u8 c2 = TestClip(poly->x2, poly->y2);

  if ((c0 & c1) == 0)
    return false;
  if ((c1 & c2) == 0)
    return false;
  if ((c2 & c0) == 0)
    return false;

  return true;
}

static inline qboolean QuadClip(const POLY_GT4 *poly) {
  // returns non-zero if a quad is outside the screen boundaries
  register const u8 c0 = TestClip(poly->x0, poly->y0);
  register const u8 c1 = TestClip(poly->x1, poly->y1);
  register const u8 c2 = TestClip(poly->x2, poly->y2);
  register const u8 c3 = TestClip(poly->x3, poly->y3);

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

static inline void *EmitAliasTriangle(POLY_FT3 *poly, const u16 tpage, const int otz, const savert_t *sv0, const savert_t *sv1, const savert_t *sv2) {
  // FT3 and GT3 have XY at the same offset, so this will work
  if (!TriClip((POLY_GT3 *)poly)) {
    setPolyFT3(poly);
    poly->tpage = tpage;
    poly->clut = rs.clut;
    addPrim(gpu_ot + otz, poly);
    ++c_draw_polys;
    ++poly;
  }
  return poly;
}

static inline void *EmitBrushTriangle(POLY_GT3 *poly, const u16 tpage, const int otz, const svert_t *sv0, const svert_t *sv1, const svert_t *sv2) {
  if (!TriClip(poly)) {
    *(u32 *)&poly->r0 = sv0->col.word;
    *(u32 *)&poly->r1 = sv1->col.word;
    *(u32 *)&poly->r2 = sv2->col.word;
    *(u16 *)&poly->u0 = sv0->tex.word;
    *(u16 *)&poly->u1 = sv1->tex.word;
    *(u16 *)&poly->u2 = sv2->tex.word;
    setPolyGT3(poly);
    poly->tpage = tpage;
    poly->clut = rs.clut;
    addPrim(gpu_ot + otz, poly);
    ++c_draw_polys;
    ++poly;
  }
  return poly;
}

static inline void *EmitBrushQuad(POLY_GT4 *poly, const u16 tpage, const s32 otz, const svert_t *sv0, const svert_t *sv1, const svert_t *sv2 , const svert_t *sv3) {
  if (!QuadClip(poly)) {
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
    poly->clut = rs.clut;
    addPrim(gpu_ot + otz, poly);
    c_draw_polys += 2;
    ++poly;
  }
  return poly;
}

// TODO: fix this properly
#pragma GCC push_options
#pragma GCC optimize("no-strict-aliasing")

static inline svert_t *HalfwayVert(svert_t *h01, const svert_t *sv0, const svert_t *sv1) {
  h01->tex.u = (sv1->tex.u + sv0->tex.u) >> 1;
  h01->tex.v = (sv1->tex.v + sv0->tex.v) >> 1;
  h01->col.r = (sv1->col.r + sv0->col.r) >> 1;
  h01->col.g = h01->col.r; // assume non-colored lighting for now
  h01->col.b = h01->col.r;
  h01->pos.x = (sv1->pos.x + sv0->pos.x) >> 1;
  h01->pos.y = (sv1->pos.y + sv0->pos.y) >> 1;
  h01->pos.z = (sv1->pos.z + sv0->pos.z) >> 1;
  return h01;
}

#define SORT_TRI(pv0, pv1, pv2, cv0, cv1, cv2) \
  gte_ldsz3(pv0->sz, pv1->sz, pv2->sz); \
  gte_avsz3(); \
  gte_stotz_m(otz); \
  if (otz > 0) { \
    *(u32 *)&poly->x0 = pv0->tpos.word; \
    *(u32 *)&poly->x1 = pv1->tpos.word; \
    *(u32 *)&poly->x2 = pv2->tpos.word; \
    poly = EmitBrushTriangle(poly, tpage, otz, cv0, cv1, cv2); \
  }

#define SORT_FTRI(pv0, pv1, pv2, cv0, cv1, cv2, az) \
  *(u32 *)&poly->x0 = pv0->tpos.word; \
  *(u32 *)&poly->x1 = pv1->tpos.word; \
  *(u32 *)&poly->x2 = pv2->tpos.word; \
  poly = EmitBrushTriangle(poly, tpage, az, cv0, cv1, cv2);

#define SORT_QUAD(pv0, pv1, pv2, pv3, cv0, cv1, cv2, cv3) \
  gte_ldsz4(pv0->sz, pv1->sz, pv2->sz, pv3->sz); \
  gte_avsz4(); \
  gte_stotz_m(otz); \
  if (otz > 0) { \
    *(u32 *)&((POLY_GT4 *)poly)->x0 = pv0->tpos.word; \
    *(u32 *)&((POLY_GT4 *)poly)->x1 = pv1->tpos.word; \
    *(u32 *)&((POLY_GT4 *)poly)->x2 = pv2->tpos.word; \
    *(u32 *)&((POLY_GT4 *)poly)->x3 = pv3->tpos.word; \
    poly = EmitBrushQuad((void *)poly, tpage, otz, cv0, cv1, cv2, cv3); \
  }

static inline POLY_GT3 *SubdivBrushTriangle1(POLY_GT3 *poly, const u16 tpage, s32 botz, svert_t *sv0, svert_t *sv1, svert_t *sv2, svert_t *clipzone) {
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

static inline POLY_GT3 *SubdivBrushTriangle2(POLY_GT3 *poly, const u16 tpage, s32 botz, svert_t *sv00, svert_t *sv01, svert_t *sv02, svert_t *clipzone) {
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

#pragma GCC pop_options

static inline POLY_GT3 *RenderBrushTriangle(POLY_GT3 *poly, const u16 tpage, svert_t *sv0, svert_t *sv1, svert_t *sv2, svert_t *clipzone) {
  register int otz;

  // calculate OT Z for the big triangle
  gte_ldsz3(sv0->sz, sv1->sz, sv2->sz);
  gte_avsz3();
  gte_stotz_m(otz);

  // clip off if out of Z range
  if (otz <= 0 || otz >= GPU_OTDEPTH)
    return poly;

  // fill in the transformed coords
  *(u32 *)&poly->x0 = sv0->tpos.word;
  *(u32 *)&poly->x1 = sv1->tpos.word;
  *(u32 *)&poly->x2 = sv2->tpos.word;

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
  const mtexture_t *mtex = R_TextureAnimation(fa->texture);
  const u16 tpage = mtex->vram_page;
  const int numverts = fa->numverts;
  svert_t *sv = PSX_SCRATCH;
  svert_t *clipzone;
  POLY_GT3 *poly = (POLY_GT3 *)gpu_ptr;
  int i;

  // put all the verts into scratch and light them; this will also ensure that pos is aligned to 4
  // that means we're limited to ~50 verts per poly, since we use the scratch to store clipped verts as well
  for (i = 0; i < numverts; ++i, ++v, ++sv) {
    sv->pos = v->pos;
    sv->tex.u = v->tex.u + mtex->vram_u;
    sv->tex.v = v->tex.v + mtex->vram_v;
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

void R_DrawVisChain(void) {
  // TODO: sky, water
  for (const msurface_t *s = rs.vischain; s; s = s->vischain)
    RenderBrushPoly(s);
}

void R_DrawAliasModel(const amodel_t *model, const int frame, const int skin, const u32 tint) {
  register int otz;
  const u16 tpage = model->skins[skin].tpage;
  const u8vec2_t baseuv = model->skins[skin].base;
  const int numverts = model->numverts;
  const int numtris = model->numtris;
  const u8vec3_t *averts = model->frames + (frame * numverts);
  POLY_FT3 *poly = (POLY_FT3 *)gpu_ptr;
  int i;

  // transform all verts first
  const u8vec3_t *av = averts;
  savert_t *sv = alias_verts;
  for (i = 0; i <= numverts - 3; i += 3, av += 3, sv += 3) {
    sv[0].pos.x = av[0].x;
    sv[0].pos.y = av[0].y;
    sv[0].pos.z = av[0].z;
    sv[1].pos.x = av[1].x;
    sv[1].pos.y = av[1].y;
    sv[1].pos.z = av[1].z;
    sv[2].pos.x = av[2].x;
    sv[2].pos.y = av[2].y;
    sv[2].pos.z = av[2].z;
    gte_ldv3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_rtpt();
    gte_stsxy3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_stsz3(&sv[0].pos.z, &sv[1].pos.z, &sv[2].pos.z);
  }
  // transform the remaining 1-2 verts
  for (; i < numverts; ++i, ++sv, ++av) {
    sv[0].pos.x = av[0].x;
    sv[0].pos.y = av[0].y;
    sv[0].pos.z = av[0].z;
    gte_ldv0(&sv[0].pos.x);
    gte_rtps();
    gte_stsxy(&sv[0].pos.x);
    gte_stsz(&sv[0].pos.z);
  }

  const atri_t *tri = model->tris;
  for (int i = 0; i < numtris; ++i, ++tri) {
    const savert_t *sv0 = &alias_verts[tri->verts[0]];
    const savert_t *sv1 = &alias_verts[tri->verts[1]];
    const savert_t *sv2 = &alias_verts[tri->verts[2]];

    // cull backfaces
    gte_ldsxy3(*(u32*)&sv0->pos.x, *(u32*)&sv1->pos.x, *(u32*)&sv2->pos.x);
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
    poly->u0 = baseuv.u + tri->uvs[0].u;
    poly->v0 = baseuv.v + tri->uvs[0].v;
    poly->u1 = baseuv.u + tri->uvs[1].u;
    poly->v1 = baseuv.v + tri->uvs[1].v;
    poly->u2 = baseuv.u + tri->uvs[2].u;
    poly->v2 = baseuv.v + tri->uvs[2].v;
    *(u32 *)&poly->r0 = tint;

    poly = EmitAliasTriangle(poly, tpage, otz, sv0, sv1, sv2);
  }

  gpu_ptr = (u8 *)poly;
}

void R_DrawAliasViewModel(const amodel_t *model, const int frame, const u32 tint) {
  register int otz;
  const u16 tpage = model->skins[0].tpage;
  const u8vec2_t baseuv = model->skins[0].base;
  const int numverts = model->numverts;
  const int numtris = model->numtris;
  const u8vec3_t *averts = model->frames + (frame * numverts);
  POLY_FT3 *poly;
  int i;

  // transform all verts first
  const u8vec3_t *av = averts;
  savert_t *sv = alias_verts;
  for (i = 0; i <= numverts - 3; i += 3, av += 3, sv += 3) {
    sv[0].pos.x = av[0].x;
    sv[0].pos.y = av[0].y;
    sv[0].pos.z = av[0].z;
    sv[1].pos.x = av[1].x;
    sv[1].pos.y = av[1].y;
    sv[1].pos.z = av[1].z;
    sv[2].pos.x = av[2].x;
    sv[2].pos.y = av[2].y;
    sv[2].pos.z = av[2].z;
    gte_ldv3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_rtpt();
    gte_stsxy3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
    gte_stsz3(&sv[0].pos.z, &sv[1].pos.z, &sv[2].pos.z);
  }
  // transform the remaining 1-2 verts
  for (; i < numverts; ++i, ++sv, ++av) {
    sv[0].pos.x = av[0].x;
    sv[0].pos.y = av[0].y;
    sv[0].pos.z = av[0].z;
    gte_ldv0(&sv[0].pos.x);
    gte_rtps();
    gte_stsxy(&sv[0].pos.x);
    gte_stsz(&sv[0].pos.z);
  }

  const atri_t *tri = model->tris;
  for (i = 0; i < numtris; ++i, ++tri) {
    const savert_t *sv0 = &alias_verts[tri->verts[0]];
    const savert_t *sv1 = &alias_verts[tri->verts[1]];
    const savert_t *sv2 = &alias_verts[tri->verts[2]];

    // cull backfaces
    gte_ldsxy3(*(u32*)&sv0->pos.x, *(u32*)&sv1->pos.x, *(u32*)&sv2->pos.x);
    gte_nclip();
    gte_stopz_m(otz);
    if (otz < 0)
      continue;

    // shit is pre-sorted
    // set positions, UVs and colors
    poly = (POLY_FT3 *)gpu_ptr;
    *(u32 *)&poly->x0 = *(u32 *)&sv0->pos.x;
    *(u32 *)&poly->x1 = *(u32 *)&sv1->pos.x;
    *(u32 *)&poly->x2 = *(u32 *)&sv2->pos.x;
    poly->u0 = baseuv.u + tri->uvs[0].u;
    poly->v0 = baseuv.v + tri->uvs[0].v;
    poly->u1 = baseuv.u + tri->uvs[1].u;
    poly->v1 = baseuv.v + tri->uvs[1].v;
    poly->u2 = baseuv.u + tri->uvs[2].u;
    poly->v2 = baseuv.v + tri->uvs[2].v;
    *(u32 *)&poly->r0 = tint;
    setPolyFT3(poly);
    poly->tpage = tpage;
    poly->clut = rs.clut;
    R_AddScreenPrim(sizeof(*poly));
  }
}

void R_DrawBrushModel(bmodel_t *model) {
  msurface_t *psurf;
  mplane_t *pplane;
  x32 dot;
  u8 side;

  psurf = &model->surfaces[model->firstmodelsurface];

  for (int i = 0; i < model->nummodelsurfaces; ++i, ++psurf) {
    // skip invisible surfaces
    if (psurf->texture->flags & (TEX_INVISIBLE | TEX_NULL))
      continue;
    // find which side of the node we are on
    pplane = psurf->plane;
    dot = XVecDotSL(&pplane->normal, &rs.modelorg) - pplane->dist;
    if (dot < -BACKFACE_EPSILON)
      side = SURF_PLANEBACK;
    else if (dot > BACKFACE_EPSILON)
      side = 0;
    else
      side = (dot < 0);
    // draw the polygon right away if it's facing the camera
    if ((side ^ psurf->backface) == 0)
      RenderBrushPoly(psurf);
  }
}

static inline LINE_F3 *DrawTwoLines(LINE_F3 *line, const int otz, const u32 col, const SVECTOR *va, const SVECTOR *vb, const SVECTOR *vc) {
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

void R_DrawBBox(edict_t *ent) {
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

static inline TILE *DrawParticle(TILE *tile, const SVECTOR *tv, const u8 color, const s16 sz) {
  // pad is sz
  if (tv->vx >= 0 && tv->vy >= 0 && tv->vx < VID_WIDTH && tv->vy < VID_HEIGHT) {
    s16 r = PT_SIZE_MIN + ((0x7FFF / sz) >> PT_SIZE_SHIFT);
    if (r > PT_SIZE_MAX) r = PT_SIZE_MAX;
    *(u32 *)&tile->r0 = r_palette[color];
    setTile(tile);
    setWH(tile, r, r);
    setXY0(tile, tv->vx - (r >> 1), tv->vy - (r >> 1));
    addPrim(gpu_ot + sz, tile);
    ++c_draw_polys;
    ++tile;
  }
  return tile;
}

void R_DrawParticles(void) {
  register s32 sz;
  SVECTOR *tv = PSX_SCRATCH;
  TILE* tile = (TILE *)gpu_ptr;

  // TODO: figure out how to transform these in threes while still respecting p->die
  particle_t *p = rs.particles;
  for (int i = 0; i < rs.num_particles; ++i, ++p) {
    if (p->die <= 0)
      continue;
    gte_ldv0(&p->org.x);
    gte_rtps();
    gte_stsxy(tv);
    gte_stsz_m(sz);
    sz >>= 2;
    if (sz > 0 && sz < GPU_OTDEPTH)
      tile = DrawParticle(tile, tv, p->color, sz);
  }

  gpu_ptr = (u8 *)tile;
}

static inline void DrawBeam(const s16vec3_t *src, const s16vec3_t *dst, const u32 rgb24) {
  register s32 otz;

  // segment the line into 3:
  // (src|sv0)--(sv1)--(sv2)--(dst|sv3)
  // sv0 - sv3 will be drawn as is, then we'll draw a copy of it with sv1 and sv2 randomly offset
  sbvert_t *sv = PSX_SCRATCH;
  sv[0].pos.x = src->x;
  sv[0].pos.y = src->y;
  sv[0].pos.z = src->z;
  sv[0].col.word = 0xFFFFFF;
  sv[3].pos.x = dst->x;
  sv[3].pos.y = dst->y;
  sv[3].pos.z = dst->z;
  sv[3].col.word = 0xFFFFFF;
  sv[1].pos.x = sv[0].pos.x + (sv[3].pos.x - sv[0].pos.x) / 3;
  sv[1].pos.y = sv[0].pos.y + (sv[3].pos.y - sv[0].pos.y) / 3;
  sv[1].pos.z = sv[0].pos.z + (sv[3].pos.z - sv[0].pos.z) / 3;
  sv[1].col.word = rgb24;
  sv[2].pos.x = (sv[3].pos.x + sv[1].pos.x) >> 1;
  sv[2].pos.y = (sv[3].pos.y + sv[1].pos.y) >> 1;
  sv[2].pos.z = (sv[3].pos.z + sv[1].pos.z) >> 1;
  sv[2].col.word = rgb24;

  // offset copies
  sv[5].pos.x = sv[2].pos.x - 8 + (rand() & 15);
  sv[5].pos.y = sv[2].pos.y - 8 + (rand() & 15);
  sv[5].pos.z = sv[2].pos.z - 8 + (rand() & 15);
  sv[5].col.word = rgb24;
  sv[6].pos.x = sv[1].pos.x - 8 + (rand() & 15);
  sv[6].pos.y = sv[1].pos.y - 8 + (rand() & 15);
  sv[6].pos.z = sv[1].pos.z - 8 + (rand() & 15);
  sv[6].col.word = rgb24;

  // offset the originals a little bit
  sv[1].pos.x += -2 + (rand() & 3);
  sv[1].pos.y += -2 + (rand() & 3);
  sv[1].pos.z += -2 + (rand() & 3);
  sv[2].pos.x += -2 + (rand() & 3);
  sv[2].pos.y += -2 + (rand() & 3);
  sv[2].pos.z += -2 + (rand() & 3);

  gte_ldv3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
  gte_rtpt();
  gte_stsxy3(&sv[0].pos.x, &sv[1].pos.x, &sv[2].pos.x);
  gte_stsz3(&sv[0].sz, &sv[1].sz, &sv[2].sz);

  gte_ldv3(&sv[3].pos.x, &sv[5].pos.x, &sv[6].pos.x);
  gte_rtpt();
  gte_stsxy3(&sv[3].pos.x, &sv[5].pos.x, &sv[6].pos.x);
  gte_stsz3(&sv[3].sz, &sv[5].sz, &sv[6].sz);

  sv[4].pos = sv[3].pos;
  sv[4].col.word = rgb24;
  sv[4].sz = sv[3].sz;
  sv[7].pos = sv[0].pos;
  sv[7].col.word = rgb24;
  sv[7].sz = sv[0].sz;

  // sort the lines separately because I can't be arsed
  LINE_F2 *line = (LINE_F2 *)gpu_ptr;
  for (int i = 0; i < 8 - 1; ++i, ++sv) {
    otz = (sv[0].sz + sv[1].sz) >> 3;
    if (otz > 0 && otz < GPU_OTDEPTH) {
      *(u32 *)&line->r0 = sv[0].col.word;
      setLineF2(line);
      setXY2(line, sv[0].pos.x, sv[0].pos.y, sv[1].pos.x, sv[1].pos.y);
      addPrim(gpu_ot + otz, line);
      line++;
    }
  }

  gpu_ptr = (u8 *)line;
}

void R_DrawBeams(void) {
  const beam_t *b = rs.beams;
  for (int i = 0; i < MAX_BEAMS; ++i, ++b)
    if (b->die > 0)
      DrawBeam(&b->src, &b->dst, b->color);
}
