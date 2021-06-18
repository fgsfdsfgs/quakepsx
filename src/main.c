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

static void TestInput(const x32 dt) {
  if (pad->stat != 0) return;

  x32vec3_t forwardmove = { 0 };
  x32vec3_t sidemove = { 0 };
  x32vec3_t upmove = { 0 };

  const x32 speed = XMUL16(200 * ONE, dt);
  const x32 aspeed = XMUL16(TO_DEG16(35), dt);

  if (!(pad->btn & PAD_UP))
    XVecScaleS(rs.vforward, speed, forwardmove)
  else if (!(pad->btn & PAD_DOWN))
    XVecScaleS(rs.vforward, -speed, forwardmove);

  if (!(pad->btn & PAD_LEFT))
    XVecScaleS(rs.vright, -speed, sidemove)
  else if (!(pad->btn & PAD_RIGHT))
    XVecScaleS(rs.vright, speed, sidemove);

  if (!(pad->btn & PAD_L1))
    XVecScaleS(rs.vup, -speed, upmove)
  else if (!(pad->btn & PAD_R1))
    XVecScaleS(rs.vup, speed, upmove);

  if (!(pad->btn & PAD_TRIANGLE))
    rs.viewangles.x -= aspeed;
  else if (!(pad->btn & PAD_CROSS))
    rs.viewangles.x += aspeed;

  if (!(pad->btn & PAD_SQUARE))
    rs.viewangles.y += aspeed;
  else if (!(pad->btn & PAD_CIRCLE))
    rs.viewangles.y -= aspeed;

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

  x32 then = 0;
  while (1) {
    then = gs.time;
    gs.time = Sys_FixedTime();
    TestInput(gs.time - then);
    R_UpdateLightStyles(gs.time);
    R_RenderView();
    R_Flip();
  }

  return 0;
}
