#pragma once

#include <psxgpu.h>

#include "common.h"
#include "entity.h"
#include "model.h"
#include "system.h"

#define BACKFACE_EPSILON 41 // F1.19.12
#define GPU_BUFSIZE 0x20000
#define GPU_OTDEPTH 1024

typedef struct render_state_s {
  RECT clip;
  edict_t *cur_entity;
  mtexture_t *cur_texture;
  x32vec3_t origin;
  x32vec3_t modelorg;
  x32vec3_t vieworg;
  x16vec3_t vforward;
  x16vec3_t vright;
  x16vec3_t vup;
  x16vec3_t viewangles;
  mplane_t frustum[4];
  mleaf_t *oldviewleaf;
  mleaf_t *viewleaf;
  u8 frame;
  u8 visframe;
  x32 frametime;
} render_state_t;

extern render_state_t rs;

extern u16 r_lightstylevalue[MAX_LIGHTSTYLES + 1];

extern int c_mark_leaves;
extern int c_draw_polys;

extern 

void *GPU_SortPrim(const u32 size, const int otz);

void R_Init(void);
void R_UploadClut(const u16 *clut);
void R_UploadTexture(const u8 *data, int x, int y, const int w, const int h);
qboolean R_CullBox(const x32vec3_t mins, const x32vec3_t maxs);
void R_SetFrustum(void);
void R_RenderScene(void);
void R_DrawWorld(void);
void R_NewMap(void);
void R_RenderBrushPoly(msurface_t *surf);
void R_RenderView(void);
void R_Flip(void);

void R_InitLightStyles(void);
void R_UpdateLightStyles(const x32 time);
void R_SetLightStyle(const int i, const char *map);
