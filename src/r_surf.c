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

static qboolean TriClip(const RECT *clip, const DVECTOR *v0, const DVECTOR *v1, const DVECTOR *v2) {
  // returns non-zero if a triangle is outside the screen boundaries
  s16 c[3];

  c[0] = TestClip(clip, v0->vx, v0->vy);
  c[1] = TestClip(clip, v1->vx, v1->vy);
  c[2] = TestClip(clip, v2->vx, v2->vy);

  if ((c[0] & c[1]) == 0)
    return false;
  if ((c[1] & c[2]) == 0)
    return false;
  if ((c[2] & c[0]) == 0)
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
  return lit | (lit << 8) | (lit << 16) | (0x34 << 24); // 0x34 = code for POLY_GT3
}

#define EMIT_BRUSH_TRIANGLE(i0, i1, i2) \
  if (otz > 0 && otz < GPU_OTDEPTH) { \
    gte_stsxy3_gt3(poly); \
    if (!TriClip(&rs.clip, (const DVECTOR *)&poly->x0, (const DVECTOR *)&poly->x1, (const DVECTOR *)&poly->x2)) { \
      setPolyGT3(poly); \
      *(u32 *)&poly->r0 = sv[i0].col; \
      *(u32 *)&poly->r1 = sv[i1].col; \
      *(u32 *)&poly->r2 = sv[i2].col; \
      *(u16 *)&poly->u0 = sv[i0].tex; \
      *(u16 *)&poly->u1 = sv[i1].tex; \
      *(u16 *)&poly->u2 = sv[i2].tex; \
      poly->tpage = tpage; \
      poly->clut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART); \
      addPrim(gpu_ot + otz, poly); \
      ++poly; \
      ++c_draw_polys; \
    } \
  }

void R_RenderBrushPoly(msurface_t *fa) {
  register const mvert_t *v = gs.worldmodel->verts + fa->firstvert;
  register const u16 tpage = fa->texture->vram_page;
  register const int numverts = fa->numverts;
  register svert_t *sv = PSX_SCRATCH;
  register POLY_GT3 *poly = (POLY_GT3 *)gpu_ptr;
  register int i, xy0, z0, otz;

  // positions have to be 8-byte aligned I think, so just put them in the scratchpad
  // in advance to avoid raping our tiny I-cache
  for (i = 0; i < numverts; ++i, ++v, ++sv) {
    sv->pos = v->pos;
    sv->tex = *(const u16 *)&v->tex;
    sv->col = LightVert(v->col, fa->styles);
  }
  // copy the first poly vert to the end to close the cycle
  *sv = ((svert_t *)PSX_SCRATCH)[1];
  // reset pointers
  sv = (svert_t *)PSX_SCRATCH;

  // load and transform first 3 verts
  gte_ldv3((SVECTOR *)&sv[0].pos, (SVECTOR *)&sv[1].pos, (SVECTOR *)&sv[2].pos);
  gte_rtpt();
  // store XYZ0 for later (Z0 is in SZ1)
  gte_stsxy0_m(xy0);
  gte_stsz1_m(z0);
  // calculate and scale OT Z
  gte_avsz3();
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
    gte_rtps();
    gte_avsz3();
    gte_stotz_m(otz);
    otz >>= GPU_OTSHIFT;
    EMIT_BRUSH_TRIANGLE(0, i - 1, i);
  }

  gpu_ptr = (void *)poly;
}

#undef EMIT_BRUSH_TRIANGLE

static inline void DrawTextureChains(void) {
  for (int i = 0; i < gs.worldmodel->numtextures; ++i) {
    mtexture_t *t = gs.worldmodel->textures + i;
    if (t->flags & TEX_INVISIBLE) continue;
    msurface_t *s = t->texchain;
    if (!s) continue;
    if (t->flags & TEX_SKY) {
      // draw sky
    } else {
      for (; s; s = s->texchain)
        R_RenderBrushPoly(s);
    }
    t->texchain = NULL;
  }
}

void R_RecursiveWorldNode(mnode_t *node) {
  if (node->contents == CONTENTS_SOLID)
    return;
  if (node->visframe != rs.visframe)
    return;
  if (R_CullBox(node->mins, node->maxs))
    return;

  // if it's a leaf node, draw stuff that's in it
  if (node->contents < 0) {
    mleaf_t *pleaf = (mleaf_t *)node;
    msurface_t **mark = pleaf->firstmarksurf;
    int c = pleaf->nummarksurf;
    if (c) {
      do {
        (*mark)->visframe = rs.frame;
        ++mark;
      } while (--c);
    }
    return;
  }

  // a node is just a decision point
  mplane_t *plane = node->plane;
  x32 dot;

  switch (plane->type) {
  case PLANE_X:
    dot = rs.modelorg.d[0] - plane->dist;
    break;
  case PLANE_Y:
    dot = rs.modelorg.d[1] - plane->dist;
    break;
  case PLANE_Z:
    dot = rs.modelorg.d[2] - plane->dist;
    break;
  default:
    dot = XVecDotSL(plane->normal, rs.modelorg) - plane->dist;
    break;
  }

  int side = !(dot >= 0);

  // recurse down the children, front side first
  R_RecursiveWorldNode(node->children[side]);

  // draw stuff in the middle
  int c = node->numsurf;
  if (c) {
    msurface_t *surf = gs.worldmodel->surfaces + node->firstsurf;
    if (dot < -BACKFACE_EPSILON)
      side = SURF_PLANEBACK;
    else if (dot > BACKFACE_EPSILON)
      side = 0;
    for (; c; --c, ++surf) {
      if (surf->visframe != rs.frame)
        continue;
      // don't backface underwater surfaces, because they warp
      if (!(surf->flags & SURF_UNDERWATER) && ((dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)))
        continue;
      // we always texsort to reduce cache misses
      surf->texchain = surf->texture->texchain;
      surf->texture->texchain = surf;
    }
  }

  // recurse down the back side
  R_RecursiveWorldNode(node->children[!side]);
}

void R_DrawWorld(void) {
  static edict_t ed;
  ed.v.model = gs.worldmodel;
  rs.modelorg = rs.vieworg;
  rs.cur_entity = &ed;
  rs.cur_texture = NULL;
  R_RecursiveWorldNode(gs.worldmodel->nodes);
  DrawTextureChains();
}
