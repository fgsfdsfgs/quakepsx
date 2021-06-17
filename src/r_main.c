#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
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
  s32 gpuot[GPU_OTDEPTH];
} fb_t;

static fb_t fb[2];
static int fbidx;

int *gpuot;
u8 *gpubuf;
u8 *gpuptr;

render_state_t rs;

int c_mark_leaves = 0;
int c_draw_polys = 0;

void *GPU_GetPtr(void) {
  return gpuptr;
}

void *GPU_SortPrim(const u32 size, const int otz) {
  u8 *newptr = gpuptr + size;
  ASSERT(gpuptr <= gpubuf + GPU_BUFSIZE);
  addPrim(gpuot + otz, gpuptr);
  gpuptr = newptr;
  return gpuptr;
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
  DrawPrim(&fill);
  fill.y0 = 256;
  DrawPrim(&fill);
  DrawSync(0);

  // set default front and back buffer
  fbidx = 0;
  gpuptr = gpubuf = fb[0].gpubuf;
  gpuot = fb[0].gpuot;
  PutDispEnv(&fb[0].disp);
  PutDrawEnv(&fb[0].draw);

  // set up GTE
  InitGeom();
  gte_SetGeomOffset(VID_CENTER_X, VID_CENTER_Y);
  gte_SetGeomScreen(VID_CENTER_X);
  gte_SetBackColor(0x80, 0x80, 0x80);

  // enable display
  SetDispMask(1);
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
  // gotta go fast
  VECTOR  *t = PSX_SCRATCH;
  SVECTOR *r = (SVECTOR *)(t + 1);
  MATRIX  *m = (MATRIX *)(r + 1);

  // rotate Z going up (thank you quake very cool)
  r->vx = TO_DEG16(+90);
  r->vy = 0;
  r->vz = TO_DEG16(+90);
  r->pad = 0;
  RotMatrix(r, &m[2]);

  // rotate according to viewangles
  r->vx = rs.viewangles.x;
  r->vy = rs.viewangles.y;
  r->vz = rs.viewangles.z;
  RotMatrix(r, &m[1]);
  // apply rotation
  MulMatrix0(&m[1], &m[2], m);

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

qboolean R_CullBox(const x32vec3_t mins, const x32vec3_t maxs) {
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
  XVecAdd(rs.vforward, rs.vright, rs.frustum[0].normal);
  XVecSub(rs.vforward, rs.vright, rs.frustum[1].normal);
  XVecAdd(rs.vforward, rs.vup, rs.frustum[2].normal);
  XVecSub(rs.vforward, rs.vup, rs.frustum[3].normal);
  for (int i = 0; i < 4; ++i) {
    rs.frustum[i].type = PLANE_ANYZ;
    rs.frustum[i].dist = XVecDotSL(rs.frustum[i].normal, rs.origin);
    rs.frustum[i].signbits = SignbitsForPlane(&rs.frustum[i]);
  }
}

void R_SetupFrame(void) {
  ++rs.frame;

  rs.origin = rs.vieworg;
  AngleVectors(rs.viewangles, &rs.vforward, &rs.vright, &rs.vup);

  rs.oldviewleaf = rs.viewleaf;
  rs.viewleaf = Mod_PointInLeaf(rs.origin, gs.worldmodel);
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
  rs.viewleaf = NULL;
  for (int i = 0; i < gs.worldmodel->numtextures; i++)
    gs.worldmodel->textures[i].texchain = NULL;
}

void R_RenderView(void) {
  if (!gs.worldmodel)
    Sys_Error("R_RenderView: NULL worldmodel");
  R_RenderScene();
}

void R_DrawDebug(void) {
  static int fontloaded = 0;
  if (!fontloaded) {
    fontloaded = 1;
    // open font
    FntLoad(960, 256);
    FntOpen(0, 8, 320, 216, 0, 255);
  }

  FntPrint(-1, "X=%04d Y=%04d Z=%04d\n", 
    rs.vieworg.x>>12, 
    rs.vieworg.y>>12, 
    rs.vieworg.z>>12);
  FntPrint(-1, "RX=%05d RY=%05d\n", 
    rs.viewangles.x, 
    rs.viewangles.y);
  FntPrint(-1, "LEAF=%05d MARK=%05d DRAW=%05d\n", rs.viewleaf - gs.worldmodel->leafs, c_mark_leaves, c_draw_polys);
  FntPrint(-1, "FWD=(%3d.%04d %3d.%04d %3d.%04d)\n",
    rs.vforward.x >> 12, rs.vforward.x & 0x0FFF,
    rs.vforward.y >> 12, rs.vforward.y & 0x0FFF,
    rs.vforward.z >> 12, rs.vforward.z & 0x0FFF);
  FntFlush(-1);
}

void R_Flip(void) {
  R_DrawDebug();

  DrawSync(0);
  VSync(0);

  fbidx ^= 1;
  gpubuf = gpuptr = fb[fbidx].gpubuf;
  gpuot = fb[fbidx].gpuot;
  ClearOTagR(gpuot, GPU_OTDEPTH);

  PutDrawEnv(&fb[fbidx].draw);
  PutDispEnv(&fb[fbidx].disp);

  DrawOTag(fb[fbidx ^ 1].gpuot + GPU_OTDEPTH - 1);

  c_draw_polys = 0;
}
