#include <stdlib.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

#include "common.h"
#include "system.h"
#include "game.h"
#include "render.h"

#define GPU_CLUT_X 0
#define GPU_CLUT_Y VID_HEIGHT

typedef struct {
  DISPENV disp;
  DRAWENV draw;
  u8 gpubuf[GPU_BUFSIZE];
  u32 gpuot[GPU_OTDEPTH];
} fb_t;

u32 *gpu_ot;
u8 *gpu_buf;
u8 *gpu_ptr;

render_state_t rs;

int c_mark_leaves = 0;
int c_draw_polys = 0;

int r_debugstream = -1;

static fb_t fb[2];
static int fbidx;

static x16 bobjrotate;

void *GPU_SortPrim(const u32 size, const int otz) {
  u8 *newptr = gpu_ptr + size;
  ASSERT(gpu_ptr <= gpu_buf + GPU_BUFSIZE);
  addPrim(gpu_ot + otz, gpu_ptr);
  gpu_ptr = newptr;
  return gpu_ptr;
}

void R_InitDebug(void) {
  FntLoad(960, 256);
  r_debugstream = FntOpen(0, 8, 320, 216, 0, 255);
}

void R_Init(void) {
  // set up double buffer
  SetDefDispEnv(&fb[0].disp, 0,   0, VID_WIDTH, VID_HEIGHT);
  SetDefDrawEnv(&fb[1].draw, 0,   0, VID_WIDTH, VID_HEIGHT);
  SetDefDispEnv(&fb[1].disp, 0, 256, VID_WIDTH, VID_HEIGHT);
  SetDefDrawEnv(&fb[0].draw, 0, 256, VID_WIDTH, VID_HEIGHT);

  setRECT(&rs.clip, 0, 0, VID_WIDTH, VID_HEIGHT);

  // enable dithering
  fb[0].draw.dtd = fb[1].draw.dtd = 1;
  // enable clearing
  fb[0].draw.isbg = fb[1].draw.isbg = 1;
  setRGB0(&fb[0].draw, 63, 0, 127);
  setRGB0(&fb[1].draw, 63, 0, 127);

  // clear both ordering tables
  ClearOTagR(fb[0].gpuot, GPU_OTDEPTH);
  ClearOTagR(fb[1].gpuot, GPU_OTDEPTH);

  // clear screen ASAP
  // need two FILL primitives because h=512 doesn't work correctly
  FILL fill = { 0 };
  setFill(&fill);
  fill.w = 512;
  fill.h = 256;
  DrawPrim((u32 *)&fill);
  fill.y0 = 256;
  DrawPrim((u32 *)&fill);
  DrawSync(0);

  // set default front and back buffer
  fbidx = 0;
  gpu_ptr = gpu_buf = fb[0].gpubuf;
  gpu_ot = fb[0].gpuot;
  PutDispEnv(&fb[0].disp);
  PutDrawEnv(&fb[0].draw);

  // set up GTE
  InitGeom();
  gte_SetGeomOffset(VID_CENTER_X, VID_CENTER_Y);
  gte_SetGeomScreen(VID_CENTER_X); // approx 90 degrees?
  gte_SetBackColor(0xFF, 0xFF, 0xFF);

  // scale Z to fit the entire range of our OT
  const register s16 zsf3 = GPU_OTDEPTH / 3;
  const register s16 zsf4 = GPU_OTDEPTH / 4;
  gte_ldzsf3_m(zsf3);
  gte_ldzsf4_m(zsf4);

  // load debug font
  R_InitDebug();

  // enable display
  SetDispMask(1);

  R_InitLightStyles();
}

void R_UploadClut(const u16 *clut) {
  static RECT clut_rect = { GPU_CLUT_X, GPU_CLUT_Y, VID_NUM_COLORS, 1 };
  LoadImage(&clut_rect, (void *)clut);
  DrawSync(0);
}

void R_UploadTexture(const u8 *data, int x, int y, const int w, const int h) {
  RECT rect = { x, y, w, h };
  LoadImage(&rect, (void *)data);
  DrawSync(0);
}

void R_SetupGPU(void) {
  // matrix that rotates Z going up (thank you quake very cool)
  static MATRIX mr = {{
    { 0,        -2 * ONE, 0        },
    { 0,        0,        -2 * ONE },
    { +2 * ONE, 0,        0        },
  }};

  // gotta go fast
  VECTOR  *t = PSX_SCRATCH;
  SVECTOR *r = (SVECTOR *)(t + 1);
  MATRIX  *m = &rs.matrix;

  // rotate according to viewangles
  r->vx = rs.viewangles.x;
  r->vy = rs.viewangles.y;
  r->vz = rs.viewangles.z;
  RotMatrix(r, &m[1]);
  // apply rotation
  MulMatrix0(&m[1], &mr, m);

  // set translation vector
  t->vx = -rs.vieworg.x >> FIXSHIFT;
  t->vy = -rs.vieworg.y >> FIXSHIFT;
  t->vz = -rs.vieworg.z >> FIXSHIFT;
  // rotate the translation vector
  ApplyMatrixLV(m, t, t);
  // apply translation to matrix
  TransMatrix(m, t);

  // put the matrix where it belongs
  gte_SetRotMatrix(m);
  gte_SetTransMatrix(m);
}

qboolean R_CullBox(const x32vec3_t *mins, const x32vec3_t *maxs) {
  for (int i = 0; i < 4; ++i)
    if (BoxOnPlaneSide(mins, maxs, &rs.frustum[i]) == 2)
      return true;
  return false;
}

static inline u8 SignbitsForPlane (mplane_t *out) {
  u8 bits = 0;
  for (int j = 0; j < 3; ++j) {
    if (out->normal.d[j] < 0)
      bits |= 1 << j;
  }
  return bits;
}

void R_SetFrustum(void) {
  // always assume FOV=90
  XVecAdd(&rs.vforward, &rs.vright, &rs.frustum[0].normal);
  XVecSub(&rs.vforward, &rs.vright, &rs.frustum[1].normal);
  XVecAdd(&rs.vforward, &rs.vup, &rs.frustum[2].normal);
  XVecSub(&rs.vforward, &rs.vup, &rs.frustum[3].normal);
  for (int i = 0; i < 4; ++i) {
    rs.frustum[i].type = PLANE_ANYZ;
    rs.frustum[i].dist = XVecDotSL(&rs.frustum[i].normal, &rs.origin);
    rs.frustum[i].signbits = SignbitsForPlane(&rs.frustum[i]);
  }
}

void R_SetupFrame(void) {
  ++rs.frame;

  rs.origin = rs.vieworg;
  AngleVectors(&rs.viewangles, &rs.vforward, &rs.vright, &rs.vup);

  rs.oldviewleaf = rs.viewleaf;
  rs.viewleaf = Mod_PointInLeaf(&rs.origin, gs.worldmodel);
}

void R_MarkLeaves(void) {
  if (rs.oldviewleaf == rs.viewleaf)
    return;

  ++rs.visframe;
  rs.oldviewleaf = rs.viewleaf;

  const u8 *vis = Mod_LeafPVS(rs.viewleaf, gs.worldmodel);
  int c = 0;
  for (int i = 0; i < gs.worldmodel->numleafs; ++i) {
    if (vis[i >> 3] & (1 << (i & 7))) {
      mnode_t *node = (mnode_t *)&gs.worldmodel->leafs[i + 1];
      do {
        if (node->visframe == rs.visframe)
          break;
        ++c;
        node->visframe = rs.visframe;
        node = node->parent;
      } while (node);
    }
  }

  c_mark_leaves = c;
}

void R_RenderScene(void) {
  R_SetupFrame();
  R_SetFrustum();
  R_SetupGPU();
  R_MarkLeaves();
  R_DrawWorld();
}

void R_NewMap(void) {
  // redownload debug font if it got overwritten
  R_InitDebug();
  rs.viewleaf = NULL;
  for (int i = 0; i < gs.worldmodel->numtextures; i++)
    gs.worldmodel->textures[i].texchain = NULL;
}

void R_RenderView(void) {
  if (!gs.worldmodel)
    Sys_Error("R_RenderView: NULL worldmodel");

  XVecAdd(&gs.player[0].ent->v.origin, &gs.player[0].viewofs, &rs.vieworg);
  rs.viewangles = gs.player[0].viewangles;

  rs.frametime = Sys_FixedTime();

  R_RenderScene();
}

void R_DrawDebug(const x32 dt) {
  player_state_t *p = gs.player;

  FntPrint(r_debugstream, "X=%04d Y=%04d Z=%04d\n",
    rs.vieworg.x>>12, 
    rs.vieworg.y>>12, 
    rs.vieworg.z>>12);
  FntPrint(r_debugstream, "VX=%04d VY=%04d VZ=%04d G=%d\n",
    p->ent->v.velocity.x>>12, 
    p->ent->v.velocity.y>>12, 
    p->ent->v.velocity.z>>12,
    (int)(p->ent->v.flags & FL_ONGROUND));
  FntPrint(r_debugstream, "RX=%05d RY=%05d\n",
    rs.viewangles.x, 
    rs.viewangles.y);
  FntPrint(r_debugstream, "L=%05d M=%05d D=%05d\n", rs.viewleaf - gs.worldmodel->leafs, c_mark_leaves, c_draw_polys);
  FntPrint(r_debugstream, "DT=%3d.%04d F=%3d VF=%3d\n", dt >> 12, dt & 4095, rs.frame, rs.visframe);
  FntFlush(r_debugstream);
}

void R_Flip(void) {
  ASSERT(gpu_ptr <= gpu_buf + GPU_BUFSIZE);

  const x32 dt = Sys_FixedTime() - rs.frametime;
  if (rs.debug)
    R_DrawDebug(dt);

  DrawSync(0);
  VSync(0);

  fbidx ^= 1;
  gpu_buf = gpu_ptr = fb[fbidx].gpubuf;
  gpu_ot = fb[fbidx].gpuot;
  ClearOTagR(gpu_ot, GPU_OTDEPTH);

  PutDrawEnv(&fb[fbidx].draw);
  PutDispEnv(&fb[fbidx].disp);

  DrawOTag(fb[fbidx ^ 1].gpuot + GPU_OTDEPTH - 1);

  c_draw_polys = 0;
}

void R_RecursiveWorldNode(mnode_t *node) {
  if (node->contents == CONTENTS_SOLID)
    return;
  if (node->visframe != rs.visframe)
    return;
  if (R_CullBox(&node->mins, &node->maxs))
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
    dot = XVecDotSL(&plane->normal, &rs.modelorg) - plane->dist;
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

static inline void DrawEntity(edict_t *ed) {
  // check if the entity is in frustum
  if (R_CullBox(&ed->v.absmin, &ed->v.absmax))
    return;

  // check if the entity is visible
  // TODO: port over the efrags system or something, this is garbage
  int i;
  for (i = 0; i < ed->num_leafs; ++i) {
    const mleaf_t *leaf = gs.worldmodel->leafs + ed->leafnums[i] + 1;
    if (leaf->visframe == rs.visframe)
      break; // found a visible leaf
  }
  if (i == ed->num_leafs)
    return; // not in any visible leaf

  rs.cur_entity = ed;
  rs.modelorg.x = ed->v.origin.x - rs.vieworg.x;
  rs.modelorg.y = ed->v.origin.y - rs.vieworg.y;
  rs.modelorg.z = ed->v.origin.z - rs.vieworg.z;

  // set up the GTE matrix
  VECTOR *t = PSX_SCRATCH;
  VECTOR *s = (VECTOR *)(t + 1);
  SVECTOR *r = (SVECTOR *)(s + 1);

  PushMatrix();

  // if this is an alias model, rotate it and add its offset and scale to the matrix
  // otherwise just translate by origin, since brush models don't rotate
  if (ed->v.modelnum < 0) {
    t->vx = ed->v.origin.x >> FIXSHIFT;
    t->vy = ed->v.origin.y >> FIXSHIFT;
    t->vz = ed->v.origin.z >> FIXSHIFT;
    TransMatrix(&rs.entmatrix, t);
  } else {
    // set rotation
    r->vx = ed->v.angles.z;
    r->vy = -ed->v.angles.x;
    r->vz = ed->v.angles.y;
    if (((amodel_t *)ed->v.model)->flags & EF_ROTATE) {
      r->vz = bobjrotate;
    } else {
      r->vz = ed->v.angles.y;
    }
    RotMatrix(r, &rs.entmatrix);
    // rotate the offset first
    t->vx = ((amodel_t *)ed->v.model)->offset.x;
    t->vy = ((amodel_t *)ed->v.model)->offset.y;
    t->vz = ((amodel_t *)ed->v.model)->offset.z;
    ApplyMatrixLV(&rs.entmatrix, t, t);
    t->vx += ed->v.origin.x >> FIXSHIFT;
    t->vy += ed->v.origin.y >> FIXSHIFT;
    t->vz += ed->v.origin.z >> FIXSHIFT;
    s->vx = ((amodel_t *)ed->v.model)->scale.x;
    s->vy = ((amodel_t *)ed->v.model)->scale.y;
    s->vz = ((amodel_t *)ed->v.model)->scale.z;
    ScaleMatrix(&rs.entmatrix, s);
    TransMatrix(&rs.entmatrix, t);
    CompMatrixLV(&rs.matrix, &rs.entmatrix, &rs.entmatrix);
    gte_SetRotMatrix(&rs.entmatrix);
  }

  gte_SetTransMatrix(&rs.entmatrix);

  // draw
  if (ed->v.modelnum < 0)
    R_DrawBrushModel(ed->v.model);
  else
    R_DrawAliasModel(ed->v.model, ed->v.frame);

  // restore the GTE matrix
  PopMatrix();
}

void R_DrawEntities(void) {
  bobjrotate = (gs.time >> 2) & (FIXSCALE - 1);

  edict_t *ed = gs.edicts;
  for (int i = 0; i <= gs.max_edict; ++i, ++ed) {
    if (ed->free || !ed->v.model)
      continue;
    DrawEntity(ed);
  }
}

void R_DrawWorld(void) {
  static edict_t ed;
  ed.v.model = gs.worldmodel;
  rs.modelorg = rs.vieworg;
  rs.cur_entity = &ed;
  rs.cur_texture = NULL;

  // first collect world surfaces into texchains
  R_RecursiveWorldNode(gs.worldmodel->nodes);

  // then draw alias entities and add BSP submodels into texchains
  R_DrawEntities();

  // then draw the texchains
  R_DrawTextureChains();
}
