#pragma once

#include <sys/types.h>
#include <psxgpu.h>
#include <psxgte.h>

#include "common.h"
#include "entity.h"
#include "model.h"
#include "sprite.h"
#include "system.h"

// some GTE macro variations that use registers instead of pointers
#define gte_stotz_m(r0)  __asm__ volatile( "mfc2   %0, $7;"  : "=r"( r0 ) : )
#define gte_stmac0_m(r0) __asm__ volatile( "mfc2   %0, $24;" : "=r"( r0 ) : )
#define gte_stsxy0_m(r0) __asm__ volatile( "mfc2   %0, $12;" : "=r"( r0 ) : )
#define gte_stsz1_m(r0)  __asm__ volatile( "mfc2   %0, $17;" : "=r"( r0 ) : )
#define gte_stopz_m(r0)  __asm__ volatile( "mfc2   %0, $24;" : "=r"( r0 ) : )
#define gte_ldsxy1_m(r0) __asm__ volatile( "mtc2   %0, $13;" : : "r"( r0 ) )
#define gte_ldsz1_m(r0)  __asm__ volatile( "mtc2   %0, $17;" : : "r"( r0 ) )
#define gte_ldsz2_m(r0)  __asm__ volatile( "mtc2   %0, $18;" : : "r"( r0 ) )
#define gte_ldzsf3_m(r0) __asm__ volatile( "mtc2   %0, $29;" : : "r"( r0 ) )
#define gte_ldzsf4_m(r0) __asm__ volatile( "mtc2   %0, $30;" : : "r"( r0 ) )

#define gte_stsz01(r1,r2) { \
  __asm__ volatile ("move  $12,%0": :"r"(r1):"$12","$13","memory"); \
  __asm__ volatile ("move  $13,%0": :"r"(r2):"$12","$13","memory"); \
  __asm__ volatile ("swc2  $17,($12)": : :"$12","$13","memory"); \
  __asm__ volatile ("swc2  $18,($13)": : :"$12","$13","memory"); \
}

#define BACKFACE_EPSILON 41 // F1.19.12
#define GPU_BUFSIZE 0x30000
#define GPU_OTDEPTH 2048

#define GPU_SUBDIV_DIST_1 80
#define GPU_SUBDIV_DIST_2 20

#define VMODEL_SCALE 3

typedef struct render_state_s {
  RECT clip;
  MATRIX matrix;
  MATRIX entmatrix;
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
  x32 frametime;
  u32 frame;
  u32 visframe;
  u32 debug;
} render_state_t;

extern render_state_t rs;

extern u16 r_lightstylevalue[MAX_LIGHTSTYLES + 1];

extern int c_mark_leaves;
extern int c_draw_polys;

extern u32 *gpu_ot;
extern u8 *gpu_buf;
extern u8 *gpu_ptr;

static inline void *R_AllocPrim(const u32 size) {
  void *ret = gpu_ptr;
  gpu_ptr += size;
  return ret;
}

void R_Init(void);
void R_UploadClut(const u16 *clut);
void R_UploadTexture(const u8 *data, int x, int y, const int w, const int h);
qboolean R_CullBox(const x32vec3_t *mins, const x32vec3_t *maxs);
void R_SetFrustum(void);
void R_AddScreenPrim(const u32 primsize);
void R_RenderScene(void);
void R_DrawWorld(void);
void R_NewMap(void);
void R_DrawAliasModel(amodel_t *model, int frame);
void R_DrawAliasViewModel(amodel_t *model, int frame);
void R_DrawBrushModel(bmodel_t *model);
void R_DrawBBox(edict_t *ent);
void R_DrawTextureChains(void);
void R_RenderView(void);
void R_Flip(void);

void R_InitLightStyles(void);
void R_UpdateLightStyles(const x32 time);
void R_SetLightStyle(const int i, const char *map);
