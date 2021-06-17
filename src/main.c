#include <psxpad.h>
#include <psxapi.h>

#include "common.h"
#include "system.h"
#include "model.h"
#include "render.h"
#include "game.h"

static char pad_buff[2][34];
static PADTYPE *pad;

static void IN_Init(void) {
  InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
  StartPAD();
  // don't make pad driver acknowledge vblank IRQ
  ChangeClearPAD(0);
  pad = (PADTYPE *)&pad_buff[0][0];
}

static void TestInput(void) {
  if (pad->stat != 0) return;

  x32vec3_t forwardmove = { 0 };
  x32vec3_t sidemove = { 0 };
  x32vec3_t upmove = { 0 };

  if (!(pad->btn & PAD_UP))
    XVecScaleS(rs.vforward, 10 * ONE, forwardmove)
  else if (!(pad->btn & PAD_DOWN))
    XVecScaleS(rs.vforward, -10 * ONE, forwardmove);

  if (!(pad->btn & PAD_LEFT))
    XVecScaleS(rs.vright, -10 * ONE, sidemove)
  else if (!(pad->btn & PAD_RIGHT))
    XVecScaleS(rs.vright, 10 * ONE, sidemove);

  if (!(pad->btn & PAD_L1))
    XVecScaleS(rs.vup, -10 * ONE, upmove)
  else if (!(pad->btn & PAD_R1))
    XVecScaleS(rs.vup, 10 * ONE, upmove);

  if (!(pad->btn & PAD_TRIANGLE))
    rs.viewangles.x -= TO_DEG16(1);
  else if (!(pad->btn & PAD_CROSS))
    rs.viewangles.x += TO_DEG16(1);

  if (!(pad->btn & PAD_SQUARE))
    rs.viewangles.y -= TO_DEG16(1);
  else if (!(pad->btn & PAD_CIRCLE))
    rs.viewangles.y += TO_DEG16(1);

  if (~(pad->btn & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT | PAD_L1 | PAD_R1))) {
    XVecAdd(rs.vieworg, forwardmove, rs.vieworg);
    XVecAdd(rs.vieworg, sidemove, rs.vieworg);
    XVecAdd(rs.vieworg, upmove, rs.vieworg);
  }
}

int main(int argc, char **argv) {
  Sys_Init();
  Mem_Init();
  R_Init();
  IN_Init();

  gs.worldmodel = Mod_LoadModel(FS_BASE "\\MAPS\\START.PSB;1");
  rs.vieworg = (x32vec3_t){ TO_FIX32(544), TO_FIX32(288), TO_FIX32(32 + 48) };
  rs.viewangles.d[YAW] = TO_DEG16(90);
  Sys_Printf("psb loaded, free mem: %u\n", Mem_GetFreeSpace());

  R_NewMap();

  while (1) {
    TestInput();
    R_RenderView();
    R_Flip();
  }

  return 0;
}
