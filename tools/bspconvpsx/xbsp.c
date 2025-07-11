#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/stb_image_write.h"

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/idmdl.h"
#include "../common/psxbsp.h"
#include "../common/psxmdl.h"
#include "../common/util.h"

#include "util.h"
#include "xbsp.h"
#include "qbsp.h"
#include "qmdl.h"
#include "qsfx.h"

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
xaliashdr_t xbsp_entmodels[MAX_XMAP_ENTMODELS];
u8          xbsp_entmodeldata[1 * 1024 * 1024];
int         xbsp_numentmodels;
u8         *xbsp_entmodeldataptr = xbsp_entmodeldata;
xmapent_t   xbsp_entities[MAX_ENTITIES];
int         xbsp_numentities;
char        xbsp_strings[MAX_XMAP_STRINGS];
int         xbsp_numstrings = 1; // actually the number of chars; empty string is always present
xmapsnd_t   xbsp_sounds[MAX_SOUNDS];
int         xbsp_numsounds;
u32         xbsp_spuptr = 0;
u16         xbsp_texatlas[VRAM_TOTAL_HEIGHT][VRAM_TOTAL_WIDTH];
xlump_t     xbsp_lumps[XLMP_COUNT];

static u8  xbsp_texbitmap[VRAM_NUM_PAGES][VRAM_PAGE_WIDTH][VRAM_PAGE_HEIGHT];
static u16 xbsp_texmaxy = 0;

static u8  xbsp_spuram[SPURAM_SIZE];

// same principle as the strings lump
static char xbsp_targetnames[MAX_XMAP_STRINGS * 2];
static int xbsp_numtargetnames = 1;

xmapsnd_t *xbsp_spu_fit(qsfx_t *src) {
  for (s32 i = 0; i < xbsp_numsounds; ++i) {
    if (xbsp_sounds[i].soundid == src->id)
      return &xbsp_sounds[i]; // already loaded
  }

  if (xbsp_numsounds >= MAX_SOUNDS)
    panic("xbsp_spu_fit(%d): too many sounds", src->id);

  if (xbsp_spuptr + 16 >= SPURAM_SIZE)
    return NULL;

  const s32 outbytes = qsfx_convert(src, xbsp_spuram + xbsp_spuptr, SPURAM_SIZE - xbsp_spuptr);
  if (outbytes <= 0)
    return NULL;

  xmapsnd_t *snd = &xbsp_sounds[xbsp_numsounds++];
  snd->spuaddr = xbsp_spuptr + SPURAM_BASE;
  snd->frames = src->numframes;
  snd->soundid = src->id;

  printf("* * id: %02x, loop: %d, pcmlen: %u, adpcmlen: %u, addr: %05x, time: %d\n",
    src->id, src->loopstart, src->numsamples * 2, outbytes, snd->spuaddr, snd->frames);

  xbsp_spuptr += outbytes;

  return snd;
}

static inline int rect_fits(const int pg, int sx, int sy, int w, int h) {
  for (int x = sx; x < sx + w; ++x) {
    for (int y = sy; y < sy + h; ++y) {
      if (xbsp_texbitmap[pg][x][y])
        return 0;
    }
  }
  return 1;
}

static inline void rect_fill(const int pg, int sx, int sy, int w, int h) {
  for (int x = sx; x < sx + w; ++x) {
    for (int y = sy; y < sy + h; ++y)
      xbsp_texbitmap[pg][x][y] = 1;
  }
}

// tries to fit w x h texture into vram image
int xbsp_vram_page_fit(xtexinfo_t *xti, const int pg, int w, int h, int *outx, int *outy) {
  int x = -1;
  int y = -1;

  // bruteforce and see if we can exactly fit anywhere
  const int xalign = 4;
  const int yalign = 1;
  for (int sx = 0; sx <= VRAM_PAGE_WIDTH - w && x < 0; sx += xalign) {
    for (int sy = 0; sy <= VRAM_PAGE_HEIGHT - h; sy += yalign) {
      // skip if crossing texpage boundary
      const int vrx = sx - (sx & 0x3C0);
      if (vrx + w > VRAM_TEXPAGE_WIDTH * 2)
        continue;
      if (rect_fits(pg, sx, sy, w, h)) {
        x = sx;
        y = sy;
        break;
      }
    }
  }

  if (x == -1 || y == -1)
    return -1;

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

int xbsp_texture_shrink(int *w, int *h, const int maxw, const int maxh) {
  int i = 0;
  while ((*w > maxw || *h > maxh) && *w > 2 && *h > 1 && i < NUM_MIPLEVELS - 1) {
    ++i;
    *w >>= 1;
    *h >>= 1;
  }
  return i;
}

int xbsp_vram_fit(const qmiptex_t *qti, xtexinfo_t *xti, int *outx, int *outy) {
  int w = qti->width;
  int h = qti->height;
  const int maxw = (xti->flags & XTEX_LARGE) ? 256 : MAX_TEX_WIDTH;
  const int maxh = (xti->flags & XTEX_LARGE) ? 256 : MAX_TEX_HEIGHT;
  int i = xbsp_texture_shrink(&w, &h, maxw, maxh);
  if (i) {
    printf("    ! larger than %dx%d, using miplevel %d\n", MAX_TEX_WIDTH, MAX_TEX_HEIGHT, i);
  }

  // width is in 16bpp pixels, we have 8-bit indexed
  w >>= 1;

  // available VRAM is organized in two pages: first 256 lines and second 256 lines
  // try fitting into the first one, if that doesn't work, try the second one
  for (i = 0; i < VRAM_NUM_PAGES; ++i) {
    if (xbsp_vram_page_fit(xti, i, w, h, outx, outy) == 0)
      return 0;
  }

  return -1;
}

void xbsp_vram_store_miptex(const qmiptex_t *qti, const xtexinfo_t *xti, int x, int y) {
  int w, h, i;
  const u8 *data;

  w = qti->width;
  h = qti->height;
  const int maxw = (xti->flags & XTEX_LARGE) ? 256 : MAX_TEX_WIDTH;
  const int maxh = (xti->flags & XTEX_LARGE) ? 256 : MAX_TEX_HEIGHT;
  i = xbsp_texture_shrink(&w, &h, maxw, maxh);
  assert(i < NUM_MIPLEVELS);

  data = (const u8 *)qti + qti->offsets[i];

  if (y + h > xbsp_texmaxy)
    xbsp_texmaxy = y + h;

  for (; h > 0; --h, ++y, data += w)
    memcpy(&xbsp_texatlas[y][x], data, w);
}

void xbsp_vram_store_mdltex(const u8 *data, int x, int y, int w, int h) {
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
  else if (!strcmp(qti->name, "quake"))
    flags |= XTEX_LARGE; // HACK: don't shrink the start.bsp quake logo as much
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
  int numverts = qf->numedges;
  const qvec2_t texsiz = {
    (qmt && qmt->width) ? qmt->width : 1,
    (qmt && qmt->height) ? qmt->height : 1
  };
  qvert_t *qverts[numverts];
  qvec2_t qst[numverts];
  u16 qlight[numverts][MAX_LIGHTMAPS];
  u32 qlightacc[MAX_LIGHTMAPS] = { 0, 0, 0, 0 };
  qvec2_t qstmin = { 999999.0f, 999999.0f };
  qvec2_t qstmax = { -99999.0f, -99999.0f };
  qvec2_t qstsiz;
  qvec2_t qlmmin, qlmsiz;
  qvec2_t quvmin, quvmax, quvsiz;

  memset(qlight, 0, numverts * sizeof(qlight[0]));

  // get verts
  for (int i = 0; i < qf->numedges; ++i) {
    const int e = qbsp->surfedges[qf->firstedge + i];
    if (e >= 0)
      qverts[i] = &qbsp->verts[qbsp->edges[e].v[0]];
    else
      qverts[i] = &qbsp->verts[qbsp->edges[-e].v[1]];
  }

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
      qst[i][j] = qdot3(qverts[i]->v, qti->vecs[j]) + qti->vecs[j][3];
      if (qst[i][j] < qstmin[j]) qstmin[j] = qst[i][j];
      if (qst[i][j] > qstmax[j]) qstmax[j] = qst[i][j];
    }
  }

  for (int j = 0; j < 2; ++j) {
    qstsiz[j] = qstmax[j] - qstmin[j];
    quvmin[j] = fsfract(qstmin[j] / texsiz[j]);
    quvsiz[j] = qstsiz[j] / texsiz[j];
    quvmax[j] = quvmin[j] + quvsiz[j];

    const f32 bmin = floorf(qstmin[j] / 16.f);
    const f32 bmax = ceilf(qstmax[j] / 16.f);
    qlmmin[j] = bmin * 16.f;
    qlmsiz[j] = (bmax - bmin) * 16.f;
  }

  for (int i = 0; i < numverts; ++i) {
    const qvert_t *qvert = qverts[i];
    qvec2_t vst = { qst[i][0], qst[i][1] };
    qvec2_t duv = { (vst[0] - qstmin[0]) / texsiz[0], (vst[1] - qstmin[1]) / texsiz[1] };
    qvec2_t fuv;

    for (int j = 0; j < 2; ++j) {
      if (quvmin[j] >= -1.0f && quvmax[j] <= 0.0f)
        fuv[j] = 1.0f + quvmin[j] + duv[j];
      else if (quvmin[j] >= 0.0f && quvmax[j] <= 1.0f)
        fuv[j] = quvmin[j] + duv[j];
      else if (quvsiz[j] <= 1.0f)
        fuv[j] = duv[j];
      else // dunno what to do, just scale it to cover the entire poly
        fuv[j] = duv[j] / quvsiz[j];
    }

    xvert_t *xv = xbsp_verts + xbsp_numverts++;
    xv->pos = qvec3_to_s16vec3(qvert->v);
    xv->tex.u = fuv[0] * 2.0f * (f32)(xti->size.u - 1);
    xv->tex.v = fuv[1] * 1.0f * (f32)(xti->size.v - 1);

    // sample lightmaps
    qbsp_light_for_vert(qbsp, qf, qvert->v, qlmmin, qlmsiz, qlight[i]);

    // accumulate contribution for each lightstyle
    for (int m = 0; m < MAX_LIGHTMAPS; ++m)
      qlightacc[m] += qlight[i][m];
  }

  // sort the lightstyles by total contribution, in descending order
  u8 lmord[MAX_LIGHTMAPS] = { 0, 1, 2, 3 };
  #define QCMP(ia, ib) \
    if (qlightacc[lmord[ia]] < qlightacc[lmord[ib]]) { u8 t = lmord[ia]; lmord[ia] = lmord[ib]; lmord[ib] = t; }
  QCMP(0, 1);
  QCMP(2, 3);
  QCMP(0, 2);
  QCMP(1, 3);
  QCMP(1, 2);
  #undef QCMP

  if (qti->flags & (TEXF_SPECIAL | TEXF_LIQUID | TEXF_SKY)) {
    // fullbright style
    xf->styles[0] = 0;
    xf->styles[1] = 0;
  } else {
    // pick the top two lightstyles
    // remap undefined style to 64, since we actually put a 0 at that index
    xf->styles[0] = qf->styles[lmord[0]] > MAX_LIGHTSTYLES ? MAX_LIGHTSTYLES : qf->styles[lmord[0]];
    xf->styles[1] = qf->styles[lmord[1]] > MAX_LIGHTSTYLES ? MAX_LIGHTSTYLES : qf->styles[lmord[1]];
  }

  // apply the selected lightstyles to all verts
  xvert_t *xv = xbsp_verts + startvert;
  for (int i = 0; i < numverts; ++i, ++xv) {
    xv->col[0] = qlight[i][lmord[0]];
    xv->col[1] = qlight[i][lmord[1]];
  }

  xf->planenum = qf->planenum;
  xf->side = qf->side;
  xf->firstvert = startvert;
  xf->numverts = numverts;
  xf->texinfo = qti->miptex;
}

static u16 xbsp_stringbuffer_add(const char *str, char *buf, int *bufnum, const int maxnum) {
  if (str == NULL || !str[0])
    return 0; // empty string

  const int len = strlen(str) + 1;

  const char *ptr = qmemsearch(buf, maxnum, str, len);
  if (ptr)
    return ptr - buf; // string already in lump

  if (*bufnum + len > maxnum)
    panic("ran out of string buffer space when adding the string: '%s'", str);

  memcpy(buf + *bufnum, str, len);

  const u16 ofs = *bufnum;
  *bufnum += len;

  return ofs;
}

u16 xbsp_string_add(const char *str) {
  char buf[8192] = { 0 };
  assert(strlen(str) < sizeof(buf) - 1);

  // map strings in Quake have escape sequences, so we need to unescape them
  // we only support \n, \r and \t
  char *dst = buf;
  for (const char *src = str; *src; ++src) {
    if (src[0] == '\\' && src[1]) {
      switch (src[1]) {
      case 'n': *dst++ = '\n'; ++src; break;
      case 'r': *dst++ = '\r'; ++src; break;
      case 't': *dst++ = '\t'; ++src; break;
      default:  *dst++ = *src; break;
      }
    } else {
      *dst++ = *src;
    }
  }
  *dst = 0;

  return xbsp_stringbuffer_add(buf, xbsp_strings, &xbsp_numstrings, sizeof(xbsp_strings));
}

u16 xbsp_string_add_upper(const char *str) {
  char tmp[256];

  if (!str || !str[0])
    return 0;

  const int len = strlen(str);
  int i;
  for (i = 0; i < len && i < 255; ++i)
    tmp[i] = toupper(str[i]);
  tmp[i] = 0;

  return xbsp_string_add(tmp);
}

u16 xbsp_targetname_id(const char *targetname) {
  return xbsp_stringbuffer_add(targetname, xbsp_targetnames, &xbsp_numtargetnames, sizeof(xbsp_targetnames));
}

void xbsp_targetname_print(void) {
  const char *t = xbsp_targetnames + 1; // 0 is an empty string
  const char *end = xbsp_targetnames + sizeof(xbsp_targetnames);
  printf("targetnames:\n");
  while (t < end && *t) {
    printf("* 0x%04x: %s\n", (u16)(t - xbsp_targetnames), t);
    t += strlen(t) + 1;
  }
}

int xbsp_write(const char *fname) {
  FILE *f = fopen(fname, "wb");
  if (!f) return -1;

  printf("writing XBSP to `%s` %d\n", fname, xbsp_texmaxy);

  xbsp_header.ver = PSXBSPVERSION;
  fwrite(&xbsp_header, sizeof(xbsp_header), 1, f);

  // write VRAM image
  fwrite(&xbsp_lumps[XLMP_TEXDATA], sizeof(xlump_t), 1, f);
  fwrite(xbsp_texatlas, sizeof(u16), VRAM_TOTAL_WIDTH * xbsp_texmaxy, f);

  // write sounds
  xsndlump_t sndlump;
  sndlump.numsfx = xbsp_numsounds;
  fwrite(&xbsp_lumps[XLMP_SNDDATA], sizeof(xlump_t), 1, f);
  fwrite(&sndlump, sizeof(sndlump), 1, f);
  fwrite(xbsp_sounds, sizeof(*xbsp_sounds), xbsp_numsounds, f);
  fwrite(xbsp_spuram, xbsp_spuptr, 1, f);

  // write alias models
  xmdllump_t mdllump;
  mdllump.nummdls = xbsp_numentmodels;
  fwrite(&xbsp_lumps[XLMP_MDLDATA], sizeof(xlump_t), 1, f);
  fwrite(&mdllump, sizeof(mdllump), 1, f);
  fwrite(xbsp_entmodels, sizeof(*xbsp_entmodels), xbsp_numentmodels, f);
  fwrite(xbsp_entmodeldata, xbsp_entmodeldataptr - xbsp_entmodeldata, 1, f);

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
  WRITE_LUMP(STRINGS,   strings,    char,        f);
  WRITE_LUMP(ENTITIES,  entities,   xmapent_t,   f);

  #undef WRITE_LUMP

  fclose(f);

  return 0;
}
