#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/psxbsp.h"
#include "../common/util.h"

#include "util.h"
#include "xbsp.h"
#include "qbsp.h"

// FIXME: removing T-junctions causes big gaps in geometry
// #define REMOVE_TJUNCTIONS 1
#define COLINEAR_EPSILON 0.001

xbsphdr_t   xbsp_header;
xvert_t     xbsp_verts[MAX_XMAP_VERTS];
int         xbsp_numverts;
xplane_t    xbsp_planes[MAX_XMAP_PLANES];
int         xbsp_numplanes;
xtexinfo_t  xbsp_texinfos[MAX_XMAP_TEXTURES];
int         xbsp_numtexinfos;
xface_t     xbsp_faces[MAX_XMAP_FACES];
int         xbsp_numfaces;
xnode_t     xbsp_nodes[MAX_XMAP_NODES];
int         xbsp_numnodes;
xclipnode_t xbsp_clipnodes[MAX_XMAP_CLIPNODES];
int         xbsp_numclipnodes;
xleaf_t     xbsp_leafs[MAX_XMAP_LEAFS];
int         xbsp_numleafs;
u16         xbsp_marksurfs[MAX_XMAP_MARKSURF];
int         xbsp_nummarksurfs;
u8          xbsp_visdata[MAX_XMAP_VISIBILITY];
int         xbsp_numvisdata;
xmodel_t    xbsp_models[MAX_XMAP_MODELS];
int         xbsp_nummodels;
xmapent_t  *xbsp_entities[MAX_ENTITIES];
int         xbsp_numentities;
xmapsnd_t  *xbsp_sounds[MAX_SOUNDS];
int         xbsp_numsounds;
u16         xbsp_clutdata[NUM_CLUT_COLORS];
u16         xbsp_texatlas[VRAM_TOTAL_HEIGHT][VRAM_TOTAL_WIDTH];
xlump_t     xbsp_lumps[XLMP_COUNT];

static u16 xbsp_texalloc[VRAM_NUM_PAGES][VRAM_PAGE_WIDTH];
static u8  xbsp_texbitmap[VRAM_NUM_PAGES][VRAM_PAGE_WIDTH][VRAM_PAGE_HEIGHT];
static u16 xbsp_texmaxy = 0;
static u32 xbsp_sndalloc = SPURAM_BASE;

xmapsnd_t *xbsp_spu_fit(const u8 *data, u32 size) {
  const u32 asize = ALIGN(size, 8);
  if (xbsp_sndalloc + asize < SPURAM_SIZE)
    panic("xbsp_spu_fit(%p, %u): could not fit", data, size);
  xmapsnd_t *snd = calloc(1, sizeof(xmapsnd_t) + size);
  assert(snd);
  snd->spuaddr = xbsp_sndalloc;
  snd->size = size;
  memcpy(snd->data, data, size);
  return snd;
}

static inline int rect_fits(const int pg, int sx, int sy, int w, int h) {
  for (int x = sx; x < sx + w; ++x)
    for (int y = sy; y < sy + h; ++y)
      if (xbsp_texbitmap[pg][x][y])
        return 0;
  return 1;
}

static inline void rect_fill(const int pg, int sx, int sy, int w, int h) {
  for (int x = sx; x < sx + w; ++x) {
    for (int y = sy; y < sy + h; ++y)
      xbsp_texbitmap[pg][x][y] = 1;
    if (xbsp_texalloc[pg][x] < sy + h)
      xbsp_texalloc[pg][x] = sy + h;
  }
}

// fits w x h texture into vram image
// main algorithm stolen from Quake2 lightmap packing
static int xbsp_vram_page_fit(xtexinfo_t *xti, const int pg, int w, int h, int *outx, int *outy) {
  int x = 0;
  int y = 0;
  int besth = VRAM_PAGE_HEIGHT;

  // HACK: bruteforce and see if we can exactly fit anywhere in already allocated space
  for (int sx = 0; sx < VRAM_PAGE_WIDTH - w; ++sx) {
    for (int sy = 0; sy < xbsp_texalloc[pg][sx] - h; ++sy) {
      if (rect_fits(pg, sx, sy, w, h)) {
        x = sx;
        y = besth = sy;
        break;
      }
    }
  }

  if (besth == VRAM_PAGE_HEIGHT) {
    for (int i = 0; i < VRAM_PAGE_WIDTH - w; ++i) {
      int th = 0;
      int j = 0;
      while (j < w) {
        if (xbsp_texalloc[pg][i + j] >= besth)
          break;
        if (xbsp_texalloc[pg][i + j] > th)
          th = xbsp_texalloc[pg][i + j];
        ++j;
      }
      if (j == w) {
        x = i;
        besth = th;
        y = th;
      }
    }

    if (besth + h > VRAM_PAGE_HEIGHT)
      return -1;
  }

  rect_fill(pg, x, y, w, h);

  x += VRAM_XSTART;
  y += VRAM_PAGE_HEIGHT * pg;

  const int vrx = x & 0x3C0;
  const int vry = y & 0x100;
  xti->tpage = PSXTPAGE(1, 0, vrx, vry);
  xti->uv.u = ((x - vrx) << 1) & 0xFF;
  xti->uv.v = y & 0xFF;
  xti->size.x = w;
  xti->size.y = h;
  if (outx) *outx = x - VRAM_XSTART;
  if (outy) *outy = y;

  return 0;
}

int xbsp_vram_fit(const qmiptex_t *qti, xtexinfo_t *xti, int *outx, int *outy) {
  int w = qti->width >> 1; // width is in 16bpp pixels, we have 8-bit indexed
  int h = qti->height;

  if (qti->width > MAX_TEX_WIDTH || qti->height > MAX_TEX_HEIGHT) {
    printf("! texture larger than %dx%d, using miplevel 1\n", MAX_TEX_WIDTH, MAX_TEX_HEIGHT);
    w >>= 1;
    h >>= 1;
  }

  // available VRAM is organized in two pages: first 256 lines and second 256 lines
  // try fitting into the first one, if that doesn't work, try the second one
  for (int i = 0; i < VRAM_NUM_PAGES; ++i) {
    if (xbsp_vram_page_fit(xti, i, w, h, outx, outy) == 0)
      return 0;
  }

  return -1;
}

void xbsp_set_palette(const u8 *pal) {
  for (int i = 0; i < NUM_CLUT_COLORS; ++i) {
    const u8 r = *pal++;
    const u8 g = *pal++;
    const u8 b = *pal++;
    xbsp_clutdata[i] = PSXRGB(r, g, b);
    if (!xbsp_clutdata[i])
      xbsp_clutdata[i] = 0x8000; // replace with non-transparent black
  }
}

void xbsp_vram_store(const qmiptex_t *qti, int x, int y) {
  int w, h;
  const u8 *data;
  if (qti->width <= MAX_TEX_WIDTH && qti->height <= MAX_TEX_HEIGHT) {
    w = qti->width;
    h = qti->height;
    data = (const u8 *)qti + qti->offsets[0];
  } else {
    w = qti->width >> 1;
    h = qti->height >> 1;
    data = (const u8 *)qti + qti->offsets[1];
  }
  if (y + h > xbsp_texmaxy)
    xbsp_texmaxy = y + h;
  for (; h > 0; --h, ++y, data += w)
    memcpy(&xbsp_texatlas[y][x], data, w);
}

void xbsp_vram_export(const char *fname, const u8 *pal) {
  u8 *rgb = malloc(2 * VRAM_TOTAL_WIDTH * VRAM_TOTAL_HEIGHT * 3);
  assert(rgb);

  const u8 *inp = (const u8 *)&xbsp_texatlas[0][0];
  u8 *outp = rgb;
  for (int y = 0; y < VRAM_TOTAL_HEIGHT; ++y) {
    for (int x = 0; x < VRAM_TOTAL_WIDTH * 2; ++x) {
      const u8 *color = &pal[*inp++ * 3];
      *outp++ = *color++;
      *outp++ = *color++;
      *outp++ = *color++;
    }
  }

  stbi_write_png(fname, VRAM_TOTAL_WIDTH * 2, VRAM_TOTAL_HEIGHT, 3, rgb, 0);

  free(rgb);
}

u16 xbsp_texture_flags(const qmiptex_t *qti) {
  u16 flags = 0;
  if (!qti)
    flags |= XTEX_NULL | XTEX_INVISIBLE | XTEX_SPECIAL;
  else if (qti->name[0] == '*')
    flags |= XTEX_SPECIAL | XTEX_LIQUID;
  else if (qti->name[0] == '+')
    flags |= XTEX_ANIMATED;
  else if (!strncmp(qti->name, "sky", 3))
    flags |= XTEX_SPECIAL | XTEX_SKY | XTEX_INVISIBLE;
  else if (!strncmp(qti->name, "clip", 16) || !strncmp(qti->name, "trigger", 16))
    flags |= XTEX_SPECIAL | XTEX_INVISIBLE;
  return flags;
}

int xbsp_vram_height(void) {
  return xbsp_texmaxy;
}

void xbsp_face_add(xface_t *xf, const qface_t *qf, const qbsp_t *qbsp) {
  const qtexinfo_t *qti = qbsp->texinfos + qf->texinfo;
  const int startvert = xbsp_numverts;
  const xtexinfo_t *xti = xbsp_texinfos + qti->miptex;
  const qmiptex_t *qmt = qbsp_get_miptex(qbsp, qti->miptex);
  int numverts = qf->numedges + 1;
  const qvec2_t texsiz = {
    (qmt && qmt->width) ? qmt->width : 1,
    (qmt && qmt->height) ? qmt->height : 1
  };
  qvert_t *qverts[numverts];
  qvec2_t qst[numverts];
  qvert_t qcent = {{ 0.f, 0.f, 0.f }};
  qvec2_t qstmin = { 999999.0f, 999999.0f };
  qvec2_t qstmax = { -99999.0f, -99999.0f };
  qvec2_t qstsiz;
  qvec2_t quvmin, quvmax, quvsiz;

  qverts[0] = &qcent;

  // get verts and calculate centroid
  // we need a vert at the centroid to subdivide stuff a little bit
  for (int i = 0; i < qf->numedges; ++i) {
    const int e = qbsp->surfedges[qf->firstedge + i];
    if (e >= 0)
      qverts[1 + i] = &qbsp->verts[qbsp->edges[e].v[0]];
    else
      qverts[1 + i] = &qbsp->verts[qbsp->edges[-e].v[1]];
    qcent.v[0] += qverts[1 + i]->v[0];
    qcent.v[1] += qverts[1 + i]->v[1];
    qcent.v[2] += qverts[1 + i]->v[2];
  }
  // average
  qcent.v[0] /= (f32)qf->numedges;
  qcent.v[1] /= (f32)qf->numedges;
  qcent.v[2] /= (f32)qf->numedges;

  // remove t-junctions if needed
#ifdef REMOVE_TJUNCTIONS
  for (int i = 0; i < numverts; ++i) {
    qvec3_t v1, v2;
    f32 *prev, *this, *next;
    prev = qverts[(i + numverts - 1) % numverts]->v;
    this = qverts[i]->v;
    next = qverts[(i + 1) % numverts]->v;
    qvec3_sub(this, prev, v1);
    qvec3_norm(v1);
    qvec3_sub(next, prev, v2);
    qvec3_norm(v2);
    if ((fabsf(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
        (fabsf(v1[1] - v2[1]) <= COLINEAR_EPSILON) && 
        (fabsf(v1[2] - v2[2]) <= COLINEAR_EPSILON)) {
      for (int j = i + 1; j < numverts; ++j)
        qverts[j - 1] = qverts[j];
      --numverts;
      // retry next vertex next time, which is now current vertex
      --i;
    }
  }
#endif

  // calculate ST for each vertex and find extents
  for (int i = 0; i < numverts; ++i) {
    // these are in image space; we'll normalize and upscale later
    for (int j = 0; j < 2; ++j) {
      qst[i][j] = qdot(qverts[i]->v, qti->vecs[j]) + qti->vecs[j][3];
      if (qst[i][j] < qstmin[j]) qstmin[j] = qst[i][j];
      if (qst[i][j] > qstmax[j]) qstmax[j] = qst[i][j];
    }
  }

  for (int j = 0; j < 2; ++j) {
    qstsiz[j] = qstmax[j] - qstmin[j];
    quvmin[j] = fsfract(qstmin[j] / texsiz[j]);
    quvsiz[j] = qstsiz[j] / texsiz[j];
    quvmax[j] = quvmin[j] + quvsiz[j];
  }

  for (int i = 0; i < numverts; ++i) {
    const qvert_t *qvert = qverts[i];
    qvec2_t vst = { qst[i][0], qst[i][1] };
    qvec2_t duv = { (vst[0] - qstmin[0]) / texsiz[0], (vst[1] - qstmin[1]) / texsiz[1] };
    qvec2_t fuv;

    for (int j = 0; j < 2; ++j) {
      if (quvmin[j] < 0.0f && quvmax[j] <= 0.0f)
        fuv[j] = 1.0f + quvmin[j] + duv[j];
      else if (quvmin[j] > 0.0f && quvmax[j] <= 1.0f)
        fuv[j] = quvmin[j] + duv[j];
      else // dunno what to do, just scale it to cover the entire poly
        fuv[j] = duv[j] / quvsiz[j];
    }

    const u16 lmcol = 0x80;
    xvert_t *xv = xbsp_verts + xbsp_numverts++;
    xv->pos = qvec3_to_s16vec3(qvert->v);
    xv->tex.u = (f32)xti->uv.u + fuv[0] * 2.0f * (f32)(xti->size.u - 1);
    xv->tex.v = (f32)xti->uv.v + fuv[1] * 1.0f * (f32)(xti->size.v - 1);

    // sample lightmaps
    u16 lit[MAX_LIGHTMAPS] = { 0, 0, 0, 0 };
    qbsp_light_for_vert(qbsp, qf, qvert->v, qstmin, qstsiz, lit);
    // assume lightstyles come without gaps
    xv->col[0] = lit[0];
    xv->col[1] = lit[1];
  }

  xf->firstvert = startvert;
  xf->numverts = numverts;
  xf->texinfo = qti->miptex;
  if ((qti->flags & TEXF_SPECIAL) || qf->lightofs < 0) {
    // fullbright style
    xf->styles[0] = 0;
    xf->styles[1] = 0;
  } else {
    // remap undefined style to 64, since we actually put a 0 at that index
    xf->styles[0] = qf->styles[0] > MAX_LIGHTSTYLES ? MAX_LIGHTSTYLES : qf->styles[0];
    xf->styles[1] = qf->styles[1] > MAX_LIGHTSTYLES ? MAX_LIGHTSTYLES : qf->styles[1];
  }
  xbsp_faces[xbsp_numfaces++] = *xf;
}

int xbsp_write(const char *fname) {
  FILE *f = fopen(fname, "wb");
  if (!f) return -1;

  printf("writing XBSP to `%s`\n", fname);

  xbsp_header.ver = PSXBSPVERSION;
  fwrite(&xbsp_header, sizeof(xbsp_header), 1, f);

  // write clut
  fwrite(&xbsp_lumps[XLMP_CLUTDATA], sizeof(xlump_t), 1, f);
  fwrite(xbsp_clutdata, sizeof(u16), NUM_CLUT_COLORS, f);

  // write VRAM image
  fwrite(&xbsp_lumps[XLMP_TEXDATA], sizeof(xlump_t), 1, f);
  fwrite(xbsp_texatlas, sizeof(u16), VRAM_TOTAL_WIDTH * xbsp_texmaxy, f);

  // write sounds
  fwrite(&xbsp_lumps[XLMP_SNDDATA], sizeof(xlump_t), 1, f);

  #define WRITE_LUMP(index, name, type, f) \
    fwrite(&xbsp_lumps[XLMP_ ## index], sizeof(xlump_t), 1, f); \
    fwrite(&xbsp_ ## name, sizeof(type), xbsp_num ## name, f)

  WRITE_LUMP(VERTS,     verts,      xvert_t,     f);
  WRITE_LUMP(PLANES,    planes,     xplane_t,    f);
  WRITE_LUMP(TEXINFO,   texinfos,   xtexinfo_t,  f);
  WRITE_LUMP(FACES,     faces,      xface_t,     f);
  WRITE_LUMP(MARKSURF,  marksurfs,  u16,         f);
  WRITE_LUMP(VISILIST,  visdata,    u8,          f);
  WRITE_LUMP(LEAFS,     leafs,      xleaf_t,     f);
  WRITE_LUMP(NODES,     nodes,      xnode_t,     f);
  WRITE_LUMP(CLIPNODES, clipnodes,  xclipnode_t, f);
  WRITE_LUMP(MODELS,    models,     xmodel_t,    f);

  #undef WRITE_LUMP

  // write entities
  fwrite(&xbsp_lumps[XLMP_ENTITIES], sizeof(xlump_t), 1, f);

  fclose(f);

  return 0;
}
