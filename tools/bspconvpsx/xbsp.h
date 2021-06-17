#pragma once

#include "../common/psxbsp.h"
#include "../common/idbsp.h"

extern xbsphdr_t   xbsp_header;
extern xvert_t     xbsp_verts[MAX_XMAP_VERTS];
extern int         xbsp_numverts;
extern xplane_t    xbsp_planes[MAX_XMAP_PLANES];
extern int         xbsp_numplanes;
extern xtexinfo_t  xbsp_texinfos[MAX_XMAP_TEXTURES];
extern int         xbsp_numtexinfos;
extern xface_t     xbsp_faces[MAX_XMAP_FACES];
extern int         xbsp_numfaces;
extern xnode_t     xbsp_nodes[MAX_XMAP_NODES];
extern int         xbsp_numnodes;
extern xclipnode_t xbsp_clipnodes[MAX_XMAP_CLIPNODES];
extern int         xbsp_numclipnodes;
extern u16         xbsp_marksurfs[MAX_XMAP_MARKSURF];
extern int         xbsp_nummarksurfs;
extern xleaf_t     xbsp_leafs[MAX_XMAP_LEAFS];
extern int         xbsp_numleafs;
extern u8          xbsp_visdata[MAX_XMAP_VISIBILITY];
extern int         xbsp_numvisdata;
extern xmodel_t    xbsp_models[MAX_XMAP_MODELS];
extern int         xbsp_nummodels;
extern xmapent_t  *xbsp_entities[MAX_ENTITIES];
extern int         xbsp_numentities;
extern xmapsnd_t  *xbsp_sounds[MAX_SOUNDS];
extern int         xbsp_numsounds;
extern u16         xbsp_clutdata[NUM_CLUT_COLORS];
extern u16         xbsp_texatlas[VRAM_TOTAL_HEIGHT][VRAM_TOTAL_WIDTH];
extern xlump_t     xbsp_lumps[XLMP_COUNT];

void xbsp_set_palette(const u8 *pal);
xmapsnd_t *xbsp_spu_fit(const u8 *data, u32 size);
int xbsp_vram_fit(const qmiptex_t *qti, xtexinfo_t *xti, int *outx, int *outy);
void xbsp_vram_store(const qmiptex_t *qti, int x, int y);
void xbsp_vram_export(const char *fname, const u8 *pal);
u16 xbsp_texture_flags(const qmiptex_t *qti);
int xbsp_vram_height(void);
void xbsp_face_add(xface_t *xf, const qface_t *qf, const qtexinfo_t *qti, const qmiptex_t *qmt, const s32 *qle, const qedge_t *qe, const qvert_t *qv);
int xbsp_write(const char *fname);
