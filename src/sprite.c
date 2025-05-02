#include "common.h"
#include "render.h"
#include "system.h"
#include "sprite.h"

#define GFX_FILE FS_BASE "\\GFX.DAT;1"

static pic_t *spr_pics = NULL;
static u16 spr_num_pics = 0;

void Spr_Init(void) {
  int fhandle;
  const int vramsize = VRAM_PIC_WIDTH * VRAM_PIC_HEIGHT * 2;
  const int fsize = Sys_FileOpenRead(GFX_FILE, &fhandle);
  if (fsize < sizeof(spr_num_pics) + vramsize)
    Sys_Error("Spr_Init: invalid GFX.DAT");

  // file starts with the default CLUT, upload it right away
  u16 *clut = Mem_Alloc(VID_NUM_COLORS * sizeof(u16));
  Sys_FileRead(fhandle, clut, VID_NUM_COLORS * sizeof(u16));
  R_UploadClut(clut);
  Mem_Free(clut);

  Sys_FileRead(fhandle, &spr_num_pics, sizeof(spr_num_pics));

  if (spr_num_pics) {
    spr_pics = Mem_Alloc(spr_num_pics * sizeof(*spr_pics));
    Sys_FileRead(fhandle, spr_pics, spr_num_pics * sizeof(*spr_pics));
    void *vramdata = Mem_Alloc(vramsize);
    Sys_FileRead(fhandle, vramdata, vramsize);
    R_UploadTexture(vramdata, VRAM_PIC_XSTART, 0, VRAM_PIC_WIDTH, VRAM_PIC_HEIGHT);
    Mem_Free(vramdata);
  }

  Sys_FileClose(fhandle);
}

const pic_t *Spr_GetPic(const int id) {
  if (!spr_pics || id <= 0 || id >= spr_num_pics)
    return NULL;
  return &spr_pics[id];
}
