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
#include "screen.h"
#include "menu.h"

#define GPU_CLUT_X 0
#define GPU_CLUT_Y VID_HEIGHT

// for drawing 2D back to front without screwing with OTs too much
typedef struct {
  u32 first;
  u8 *last;
} plist_t;

typedef struct {
  DISPENV disp;
  DRAWENV draw;
  plist_t gpuplist;
  u8 gpubuf[GPU_BUFSIZE];
  u32 gpuot[GPU_OTDEPTH];
} fb_t;

u32 *gpu_ot;
u8 *gpu_buf;
u8 *gpu_ptr;
plist_t *gpu_plist;

render_state_t rs;

u32 r_palette[VID_NUM_COLORS]; // used for particles, etc

int c_mark_leaves = 0;
int c_draw_polys = 0;

static fb_t fb[2];
static int fbidx;

static x16 bobjrotate;
static x32 bob_t = 0;

// matrix that rotates Z going up and also scales the world a little (thank you quake very cool)
static MATRIX mat_coord = {{
  { 0,        -2 * ONE, 0        },
  { 0,        0,        -2 * ONE },
  { +2 * ONE, 0,        0        },
}};

static inline void DrawEntity(edict_t *ed);

static inline void Plist_Append(const u32 size) {
  if (!gpu_plist->last)
    catPrim(&gpu_plist->first, gpu_ptr);
  else
    catPrim(gpu_plist->last, gpu_ptr);
  gpu_plist->last = gpu_ptr;
  gpu_ptr += size;
}

static inline void Plist_Clear(void) {
  gpu_plist->first = 0xFFFFFF;
  gpu_plist->last = NULL;
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
  gpu_plist = &fb[0].gpuplist;
  PutDispEnv(&fb[0].disp);
  PutDrawEnv(&fb[0].draw);
  Plist_Clear();

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

  R_InitLightStyles();

  // load gfx.dat
  Spr_Init();

  Scr_Init();

  // enable display
  SetDispMask(1);
}

void R_UploadClut(const u16 *clut) {
  static RECT clut_rect = { GPU_CLUT_X, GPU_CLUT_Y, VID_NUM_COLORS, 1 };
  LoadImage(&clut_rect, (void *)clut);
  DrawSync(0);

  // store an RGBA8888 copy of it for particles and the like
  u32 *dst = r_palette;
  for (int i = 0; i < VID_NUM_COLORS; ++i, ++dst, ++clut) {
    const u16 c = *clut;
    const u8 r = (c & 31) << 3;
    const u8 g = ((c >> 5) & 31) << 3;
    const u8 b = ((c >> 10) & 31) << 3;
    *dst = r | (g << 8) | (b << 16);
  }
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
  MATRIX *tm = (MATRIX *)(r + 1);
  MATRIX  *m = &rs.matrix;

  // rotate according to viewangles
  r->vx = rs.viewangles.x;
  r->vy = rs.viewangles.y;
  r->vz = rs.viewangles.z;
  RotMatrix(r, tm);
  // apply rotation
  MulMatrix0(tm, &mat_coord, m);

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

void R_DrawViewModel(const player_state_t *plr) {
  if (!plr->vmodel || plr->ent->v.health <= 0)
    return;

  plr->ent->num_leafs = 1;
  plr->ent->leafnums[0] = rs.viewleaf - gs.worldmodel->leafs - 1;
  R_LightEntity(plr->ent);

  x32vec3_t delta;
  XVecSub(&plr->ent->v.origin, &plr->ent->v.oldorigin, &delta);

  x32vec2_t absspeed = { abs(plr->ent->v.velocity.x), abs(plr->ent->v.velocity.y) };
  bob_t += xmul32((absspeed.x > absspeed.y ? absspeed.x : absspeed.y), gs.frametime) >> (FIXSHIFT - 5);

  const x32 bob_x = rsin(bob_t);

  VECTOR *t = PSX_SCRATCH;
  VECTOR *s = (VECTOR *)(t + 1);
  MATRIX *m = (MATRIX *)(s + 1);

  *m = mat_coord;

  s->vx = (x32)plr->vmodel->scale.x << VMODEL_SCALE;
  s->vy = (x32)plr->vmodel->scale.y << VMODEL_SCALE;
  s->vz = (x32)plr->vmodel->scale.z << VMODEL_SCALE;
  ScaleMatrix(m, s);

  t->vx = plr->vmodel->offset.x;
  t->vx = (t->vx << VMODEL_SCALE) + (bob_x >> 9);
  t->vy = plr->vmodel->offset.y;
  t->vy = t->vy << VMODEL_SCALE;
  t->vz = plr->vmodel->offset.z;
  t->vz = t->vz << VMODEL_SCALE;
  ApplyMatrixLV(&mat_coord, t, t);
  TransMatrix(m, t);

  gte_SetRotMatrix(m);
  gte_SetTransMatrix(m);

  const u32 tint = (plr->ent->v.light << 16) | (plr->ent->v.light << 8) | (plr->ent->v.light);
  R_DrawAliasViewModel(plr->vmodel, plr->vmodel_frame, tint);

  PopMatrix();
}

void R_RenderScene(void) {
  R_SetupFrame();
  R_SetFrustum();
  R_SetupGPU();
  R_MarkLeaves();
  R_DrawWorld();
  R_UpdateParticles();
  R_DrawParticles();
}

void R_NewMap(void) {
  rs.viewleaf = NULL;
  rs.oldviewleaf = NULL;
  rs.cur_entity = NULL;
  rs.cur_texture = NULL;
  rs.visframe = 0;
  rs.frame = 0;
  rs.frametime = 0;
  for (int i = 0; i < gs.worldmodel->numtextures; i++)
    gs.worldmodel->textures[i].texchain = NULL;
}

void R_RenderView(void) {
  if (!gs.worldmodel)
    return;

  const player_state_t *plr = &gs.player[0];

  XVecAdd(&plr->ent->v.origin, &plr->viewofs, &rs.vieworg);

  rs.viewangles.x = plr->viewangles.x + plr->punchangle;
  rs.viewangles.y = plr->viewangles.y;
  rs.viewangles.z = plr->viewangles.z;

  rs.frametime = Sys_FixedTime();

  R_RenderScene();

  R_DrawViewModel(plr);

  if (menu_open)
    Menu_Draw();
  else
    Scr_DrawScreen(rs.debug);
}

void R_Flip(void) {
  ASSERT(gpu_ptr <= gpu_buf + GPU_BUFSIZE);

  const x32 dt = Sys_FixedTime() - rs.frametime;

  DrawSync(0);
  VSync(0);

  fbidx ^= 1;
  gpu_buf = gpu_ptr = fb[fbidx].gpubuf;
  gpu_ot = fb[fbidx].gpuot;
  ClearOTagR(gpu_ot, GPU_OTDEPTH);

  PutDrawEnv(&fb[fbidx].draw);
  PutDispEnv(&fb[fbidx].disp);

  // if we drew any 2D stuff this frame, concat it to the beginning of the OT,
  // so it draws on top of everything
  if (gpu_plist->last) {
    termPrim(gpu_plist->last);
    catPrim(fb[fbidx ^ 1].gpuot, &gpu_plist->first);
  }

  DrawOTag(fb[fbidx ^ 1].gpuot + GPU_OTDEPTH - 1);

  gpu_plist = &fb[fbidx].gpuplist;
  Plist_Clear();

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
  ed->v.flags &= ~FL_VISIBLE;

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

  // just rendered
  ed->v.flags |= FL_VISIBLE;

  // TODO: not sure where to move this
  if (ed->v.effects & (EF_GIB | EF_ROCKET)) {
    if (ed->v.velocity.x || ed->v.velocity.y || ed->v.velocity.z)
      R_SpawnParticleTrail(&ed->v.origin, &ed->v.oldorigin, (ed->v.effects & EF_GIB) ? 2 : 0);
    else
      ed->v.effects &= ~(EF_GIB | EF_ROCKET);
  }

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
    rs.entmatrix = (MATRIX){{
      ONE, 0,   0,
      0,   ONE, 0,
      0,   0,   ONE
    }, {
      ed->v.origin.x >> FIXSHIFT,
      ed->v.origin.y >> FIXSHIFT,
      ed->v.origin.z >> FIXSHIFT
    }};
  } else {
    // set rotation
    r->vx = ed->v.angles.z;
    r->vy = ed->v.angles.x;
    if (((amodel_t *)ed->v.model)->flags & EF_ROTATE) {
      r->vz = bobjrotate;
    } else {
      r->vz = ed->v.angles.y;
    }
    RotMatrixZY(r, &rs.entmatrix);
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
  }

  CompMatrixLV(&rs.matrix, &rs.entmatrix, &rs.entmatrix);
  gte_SetRotMatrix(&rs.entmatrix);
  gte_SetTransMatrix(&rs.entmatrix);

  // draw
  if (ed->v.modelnum < 0) {
    R_DrawBrushModel(ed->v.model);
  } else {
    R_LightEntity(ed);
    R_DrawAliasModel(ed->v.model, ed->v.frame, (ed->v.light << 16) | (ed->v.light << 8) | (ed->v.light));
  }

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

  // then draw the texchains
  R_DrawTextureChains();

  // then draw alias entities and BSP submodels
  R_DrawEntities();
}

void R_AddScreenPrim(const u32 primsize) {
  Plist_Append(primsize);
}

void R_DrawBlitSync(const pic_t *pic, int x, const int y) {
  RECT src = { 0, 0, pic->size.u, pic->size.v };
  src.x = (pic->tpage & 0x0f) * 64 + pic->uv.u;
  src.y = (pic->tpage & 0x10) * 16 + pic->uv.v;
  MoveImage2(&src, x + fb[fbidx].disp.disp.x, y + fb[fbidx].disp.disp.y);
}
