#include <stdlib.h>
#include <string.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <inline_c.h>

#include "common.h"
#include "system.h"
#include "render.h"
#include "entity.h"
#include "game.h"

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM	8

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

static inline qboolean TriClip(const RECT *clip, const DVECTOR *v0, const DVECTOR *v1, const DVECTOR *v2) {
  // returns non-zero if a triangle is outside the screen boundaries
  register s16 c0, c1, c2;

  c0 = TestClip(clip, v0->vx, v0->vy);
  c1 = TestClip(clip, v1->vx, v1->vy);
  c2 = TestClip(clip, v2->vx, v2->vy);

  if ((c0 & c1) == 0)
    return false;
  if ((c1 & c2) == 0)
    return false;
  if ((c2 & c0) == 0)
    return false;

  return true;
}

static inline qboolean QuadClip(const RECT *clip, const DVECTOR *v0, const DVECTOR *v1, const DVECTOR *v2, const DVECTOR *v3) {
  // returns non-zero if a quad is outside the screen boundaries
  register s16 c0, c1, c2, c3;

  c0 = TestClip(clip, v0->vx, v0->vy);
  c1 = TestClip(clip, v1->vx, v1->vy);
  c2 = TestClip(clip, v2->vx, v2->vy);
  c3 = TestClip(clip, v3->vx, v3->vy);

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

typedef struct {
  s16vec3_t pos;
  u16 pad0;
  u32 col;
  u16 tex;
  u16 pad1;
} svert_t;

static inline u32 LightVert(const u8 *col, const u8 *styles) {
  register u32 lit;
  lit  = col[0] * r_lightstylevalue[styles[0]];
  lit += col[1] * r_lightstylevalue[styles[1]];
  lit  = (lit >> 8);
  return lit | (lit << 8) | (lit << 16);
}

#define EMIT_BRUSH_TRIANGLE(i0, i1, i2) \
  if (otz > 0 && otz < GPU_OTDEPTH) { \
    gte_stsxy3_gt3(poly); \
    if (!TriClip(&rs.clip, (const DVECTOR *)&poly->x0, (const DVECTOR *)&poly->x1, (const DVECTOR *)&poly->x2)) { \
      *(u32 *)&poly->r0 = sv[i0].col; \
      *(u32 *)&poly->r1 = sv[i1].col; \
      *(u32 *)&poly->r2 = sv[i2].col; \
      *(u16 *)&poly->u0 = sv[i0].tex; \
      *(u16 *)&poly->u1 = sv[i1].tex; \
      *(u16 *)&poly->u2 = sv[i2].tex; \
      setPolyGT3(poly); \
      poly->tpage = tpage; \
      poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART); \
      addPrim(gpu_ot + otz, poly); \
      ++poly; \
      ++c_draw_polys; \
    } \
  }

static inline void RenderBrushPoly(msurface_t *fa) {
  register const mvert_t *v = gs.worldmodel->verts + fa->firstvert;
  register const u16 tpage = fa->texture->vram_page;
  register const int numverts = fa->numverts;
  register svert_t *sv = PSX_SCRATCH;
  register int i;

  // positions have to be 8-byte aligned I think, so just put them in the scratchpad
  // in advance to avoid raping our tiny I-cache
  for (i = 0; i < numverts; ++i, ++v, ++sv) {
    sv->pos = v->pos;
    sv->tex = *(const u16 *)&v->tex;
    sv->col = LightVert(v->col, fa->styles);
  }
  // reset pointers
  sv = (svert_t *)PSX_SCRATCH;

  // quads are rendered as is, everything else is a triangle fan
  if (numverts == 4) {
    // it's a quad
    register POLY_GT4 *poly = (POLY_GT4 *)gpu_ptr;
    // load and transform first 3 verts
    gte_ldv3((SVECTOR *)&sv[0].pos, (SVECTOR *)&sv[3].pos, (SVECTOR *)&sv[1].pos);
    gte_rtpt_b();
    // store them out
    gte_stsxy3_gt4(poly);
    // transform last vertex
    gte_ldv0((SVECTOR *)&sv[2].pos);
    gte_rtps_b();
    gte_stsxy((DVECTOR *)&poly->x3);
    // emit quad if it's in range
    if (!QuadClip(&rs.clip, (DVECTOR *)&poly->x0, (DVECTOR *)&poly->x1, (DVECTOR *)&poly->x2, (DVECTOR *)&poly->x3)) {
      gte_avsz4_b();
      gte_stotz_m(i);
      i >>= GPU_OTSHIFT;
      if (i > 0 && i < GPU_OTDEPTH) {
        *(u32 *)&poly->r0 = sv[0].col;
        *(u32 *)&poly->r1 = sv[3].col;
        *(u32 *)&poly->r2 = sv[1].col;
        *(u32 *)&poly->r3 = sv[2].col;
        *(u16 *)&poly->u0 = sv[0].tex;
        *(u16 *)&poly->u1 = sv[3].tex;
        *(u16 *)&poly->u2 = sv[1].tex;
        *(u16 *)&poly->u3 = sv[2].tex;
        setPolyGT4(poly); // gotta do it after colors to fill in ->code
        poly->tpage = tpage;
        poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
        addPrim(gpu_ot + i, poly);
        ++poly;
        ++c_draw_polys;
      }
    }
    gpu_ptr = (void *)poly;
  } else {
    // it's a triangle fan
    register POLY_GT3 *poly = (POLY_GT3 *)gpu_ptr;
    register int xy0, z0, otz;
    // close the loop
    sv[numverts] = ((svert_t *)PSX_SCRATCH)[1];
    // load and transform first 3 verts
    gte_ldv3((SVECTOR *)&sv[0].pos, (SVECTOR *)&sv[1].pos, (SVECTOR *)&sv[2].pos);
    gte_rtpt_b();
    // store XYZ0 for later (Z0 is in SZ1)
    gte_stsxy0_m(xy0);
    gte_stsz1_m(z0);
    // calculate and scale OT Z
    gte_avsz3_b();
    gte_stotz_m(otz);
    otz >>= GPU_OTSHIFT;
    // emit first triangle if it's in range
    EMIT_BRUSH_TRIANGLE(0, 1, 2);
    // now we only have to transform 1 extra vert each iteration
    // RTPS will push the previous vertices back, so we'll need to restore XYZ0 into slot 1
    for (i = 3; i <= numverts; ++i) {
      gte_ldsxy1_m(xy0);
      gte_ldsz2_m(z0);
      gte_ldv0((SVECTOR *)&sv[i].pos);
      gte_rtps_b();
      gte_avsz3();
      gte_stotz_m(otz);
      otz >>= GPU_OTSHIFT;
      EMIT_BRUSH_TRIANGLE(0, i - 1, i);
    }
    gpu_ptr = (void *)poly;
  }
}

#undef EMIT_BRUSH_TRIANGLE

void R_DrawTextureChains(void) {
  for (int i = 0; i < gs.worldmodel->numtextures; ++i) {
    mtexture_t *t = gs.worldmodel->textures + i;
    if (t->flags & TEX_INVISIBLE) continue;
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
