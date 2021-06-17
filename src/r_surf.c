#include <stdlib.h>
#include <string.h>
#include <psxgpu.h>
#include <psxgte.h>

#include "common.h"
#include "system.h"
#include "render.h"
#include "entity.h"
#include "game.h"

#define CLIP_LEFT	1
#define CLIP_RIGHT	2
#define CLIP_TOP	4
#define CLIP_BOTTOM	8

static inline int TestClip(const RECT *clip, const s16 x, const s16 y) {
  // tests which corners of the screen a point lies outside of
  int result = 0;
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

void R_RenderBrushPoly(msurface_t *fa) {
  const mvert_t *v = gs.worldmodel->verts + fa->firstvert;
  const u16 tpage = fa->texture->vram_page;
  const u16 tclut = getClut(VRAM_PAL_XSTART, VRAM_PAL_YSTART);
  // positions have to be 8-byte aligned I think, so just put them in the scratchpad
  SVECTOR *pos = PSX_SCRATCH;
  pos[0].vx = v[0].pos.x;
  pos[0].vy = v[0].pos.y;
  pos[0].vz = v[0].pos.z;

  POLY_GT3 *p = GPU_GetPtr();
  int otz = 0;

  for (u16 i = 1; i < fa->numverts - 1; ++i) {
    // copy positions to aligned buffer
    pos[1].vx = v[i].pos.x;
    pos[1].vy = v[i].pos.y;
    pos[1].vz = v[i].pos.z;
    pos[2].vx = v[i + 1].pos.x;
    pos[2].vy = v[i + 1].pos.y;
    pos[2].vz = v[i + 1].pos.z;

    // load verts
    gte_ldv3(&pos[0], &pos[1], &pos[2]);

    // transform verts
    gte_rtpt();

    // backface cull if needed
    gte_nclip();
    gte_stopz(&otz);
    // even though they're all in the same plane, we can't just bail here
    // if we do, some tris end up missing, presumably because of inaccuracy
    if (otz < 0)
      continue;

    // calculate OT Z
    gte_avsz3();
    gte_stotz(&otz);

    // reject polys outside of Z range
    otz >>= 2;
    if (otz <= 0 || otz >= GPU_OTDEPTH)
      continue;

    // initialize primitive with computed vertices
    setPolyGT3(p);
    gte_stsxy3_gt3(p);

    // check if it's outside of the screen
    if (TriClip(&rs.clip, (DVECTOR *)&p->x0, (DVECTOR *)&p->x1, (DVECTOR *)&p->x2))
      continue;

    // set color
    setSemiTrans(p, 0);
    setRGB0(p, v[0].col, v[0].col, v[0].col);
    setRGB1(p, v[i].col, v[i].col, v[i].col);
    setRGB2(p, v[i + 1].col, v[i + 1].col, v[i + 1].col);

    // set uv
    p->u0 = v[0].tex.u;
    p->v0 = v[0].tex.v;
    p->u1 = v[i].tex.u;
    p->v1 = v[i].tex.v;
    p->u2 = v[i + 1].tex.u;
    p->v2 = v[i + 1].tex.v;
    p->tpage = tpage;
    p->clut = tclut;

    // sort it into the OT
    p = GPU_SortPrim(sizeof(POLY_GT3), otz);
    ++c_draw_polys;
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
