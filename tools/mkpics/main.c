#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/stb_image_write.h"

#include "../common/util.h"
#include "../common/pak.h"
#include "../common/wad.h"
#include "../common/psxbsp.h"

#define CONCHARS_SIZE 128
#define MAX_PICS 256
#define MAX_LINE 2048
#define MAX_PIC_NAME 128
#define PICS_XSTART 960
#define PICS_XEND 1023

static u8 *paldata = NULL;
static u16 clutdata[NUM_CLUT_COLORS];
static u16 vramdata[VRAM_TOTAL_HEIGHT][VRAM_TEXPAGE_WIDTH];

static wad_t gfxwad;

static u16 num_pics = 1;
static xpic_t pics[MAX_PICS];
static char picnames[MAX_PICS][MAX_PIC_NAME] = { "NONE" };

static const char *moddir;
static const char *cfgdir;
static const char *dstdat;
static const char *dsthdr;

static inline void load_palette(void) {
  const char *palname = "gfx/palette.lmp";
  size_t palsize = 0;
  paldata = lmp_read(moddir, palname, &palsize);

  if (palsize < NUM_CLUT_COLORS * 3)
    panic("palette too small: %d bytes", (s32)palsize);

  convert_palette(clutdata, paldata, NUM_CLUT_COLORS);
}

static inline void load_gfxwad(void) {
  const char *wadname = "gfx.wad";
  size_t wadsize = 0;
  u8 *waddata = lmp_read(moddir, wadname, &wadsize);

  if (wad_open(&gfxwad, waddata, wadsize) < 0)
    panic("could not open %s in moddir %s", wadname, moddir);
}

static inline void process_pic(const char *name, const int x, const int y, const int ckey, const int rgb555) {
  size_t size = 0;
  u8 *pixels = NULL;
  qpic_t *pic = NULL;
  s32 w = 0;
  s32 h = 0;
  s32 w16 = 0;

  if (num_pics >= MAX_PICS)
    panic("too many pics (max %d)", MAX_PICS);

  if (!strcmp(name, "conchars")) {
    // conchars is a special case; has no header
    pixels = wad_get_lump(&gfxwad, "CONCHARS", &size);
    w = CONCHARS_SIZE;
    h = CONCHARS_SIZE;
  } else if (strchr(name, '/') || strchr(name, '.')) {
    // pak pic
    pic = lmp_read(moddir, name, &size);
    if (pic) {
      pixels = pic->data;
      w = pic->width;
      h = pic->height;
    }
  } else {
    // wad pic
    pic = (qpic_t *)wad_get_lump(&gfxwad, name, &size);
    if (pic) {
      pixels = pic->data;
      w = pic->width;
      h = pic->height;
      pic = NULL; // don't need to free
    }
  }

  w16 = rgb555 ? w : (w >> 1);

  if (!pixels || !size || w <= 0 || h <= 0)
    panic("no such pic or empty pic: %s", name);

  if (ckey < 0 || ckey >= NUM_CLUT_COLORS)
    panic("pic %s has colorkey out of bounds: %d", ckey);

  if (w >= 256 || h >= 256)
    panic("pic %s is too big: %dx%d", name, w, h);

  if ((x & 1) || (w & 1))
    panic("pic %s has odd width or x: x=%d, w=%d", name, w);

  if (x < PICS_XSTART || x > PICS_XEND)
    panic("pic %s has x out of bounds: %d", x);

  if (y < 0 || y >= VRAM_TOTAL_HEIGHT)
    panic("pic %s has y out of bounds: %d", y);

  const int basex = (x & 0x3C0);
  const int basey = (y & 0x100);
  if (x < 0 || (w16 + x - basex > VRAM_TEXPAGE_WIDTH))
    panic("pic %s intersects tpage X boundary", name);
  if (y < 0 || (h + y - basey > VRAM_TEXPAGE_HEIGHT))
    panic("pic %s intersects tpage Y boundary", name);

  // convert or remap colorkey if needed
  if (rgb555) {
    u16 *newpixels = calloc(w * h, 2);
    assert(newpixels);
    for (int i = 0; i < w * h; ++i)
      newpixels[i] = clutdata[pixels[i]];
    pixels = (u8 *)newpixels;
  } else if (ckey != 0xFF) {
    for (int i = 0; i < w * h; ++i) {
      if (pixels[i] == ckey)
        pixels[i] = 0xFF;
    }
  }

  // copy the data
  const u8 *src = pixels;
  const s32 pitch = w  * (1 + rgb555);
  for (int i = 0; i < h; ++i, src += pitch)
    memcpy(&vramdata[y + i][x - basex], src, pitch);

  xpic_t *xpic = &pics[num_pics];
  xpic->size.x = w;
  xpic->size.y = h;
  xpic->uv.u = ((x - basex) << !rgb555) & 0xFF;
  xpic->uv.v = y & 0xFF;
  xpic->tpage = PSXTPAGE(1, 0, x, y);
  snprintf(picnames[num_pics], sizeof(picnames[num_pics]), "%s", name);

  printf("pic #%02x: pos (%4d, %3d), size (%3d, %3d), uv (%3d, %3d), key %02x, name %s, %dbit\n",
    num_pics, x, y, w, h, xpic->uv.u, xpic->uv.v, (u32)ckey, name, rgb555 ? 16 : 8);

  ++num_pics;

  free(pic);
  if (rgb555)
    free(pixels);
}

static inline void parse_picmap(const char *fname) {
  FILE *f = fopen(fname, "rb");
  if (!f) panic("can't open picmap file %s", fname);

  char line[MAX_LINE] = { 0 };
  const char *delim = " \t\r\n";
  while (fgets(line, sizeof(line), f)) {
    const char *p = strtok(line, delim);
    if (!p)
      continue;

    const int ispic = strcmp(p, "pic") == 0;
    const int ispic16 = strcmp(p, "pic16") == 0;
    if (!ispic && !ispic16)
      continue;

    char *name = strtok(NULL, delim);
    const char *sx = strtok(NULL, delim);
    const char *sy = strtok(NULL, delim);
    const char *scmap = strtok(NULL, delim);
    if (!name || !sx || !sy || !isdigit(sx[0]) || !isdigit(sy[0]))
      panic("malformed line in %s:\n%s", fname, line);

    const int vx = atoi(sx);
    const int vy = atoi(sy);

    int cmap = 0xFF; // in quake the last color is transparent
    if (scmap && isdigit(scmap[0]))
      cmap = atoi(scmap);

    process_pic(name, vx, vy, cmap, ispic16);
  }

  fclose(f);
}

static inline const char *pic_name(const int i) {
  char *p = strchr(picnames[i], '/');
  p = p ? p + 1 : picnames[i];
  char *dot = strchr(p, '.');
  if (dot)
    *dot = '\0';
  for (char *pp = p; *pp; ++pp)
    *pp = toupper(*pp);
  return p;
}

static void cleanup(void) {
  wad_close(&gfxwad);
  free(paldata);
  paldata = NULL;
}

int main(int argc, const char **argv) {
  if (argc < 4 || argc > 5) {
    printf("usage: %s <moddir> <cfgdir> <outdat> [<outhdr>]\n", argv[0]);
    return -1;
  }

  moddir = argv[1];
  cfgdir = argv[2];
  dstdat = argv[3];
  dsthdr = argc > 4 ? argv[4] : NULL;

  load_palette();
  load_gfxwad();

  atexit(cleanup);

  parse_picmap(strfmt("%s/picmap.txt", cfgdir));

  printf("read %d pics\n", num_pics);

  const char *outfile = dstdat;
  printf("saving VRAM bank to %s\n", outfile);
  FILE *f = fopen(outfile, "wb");
  if (!f) panic("could not open %s for writing", outfile);
  fwrite(clutdata, sizeof(clutdata), 1, f);
  fwrite(&num_pics, sizeof(num_pics), 1, f);
  fwrite(pics, sizeof(pics[0]), num_pics, f);
  fwrite(vramdata, sizeof(vramdata), 1, f);
  fclose(f);

  if (dsthdr) {
    outfile = dsthdr;
    printf("saving pics enum to %s\n", outfile);
    f = fopen(outfile, "wb");
    if (!f) panic("could not open %s for writing", outfile);
    fprintf(f, "#pragma once\n\nenum {\n");
    for (int i = 0; i < num_pics; ++i)
      fprintf(f, "  PICID_%s = 0x%02x,\n", pic_name(i), i);
    fprintf(f, "};\n");
    fclose(f);
  }

  return 0;
}
