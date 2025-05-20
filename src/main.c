#include "common.h"
#include "system.h"
#include "cd.h"
#include "render.h"
#include "input.h"
#include "game.h"
#include "sound.h"
#include "profile.h"

int main(int argc, char **argv) {
  Mem_Init();
  Sys_Init();
  IN_Init();
  CD_Init();
  Snd_Init();
  R_Init();

  Mem_SetMark(MEM_MARK_LO);

  // default to normal skill
  gs.skill = 1;

  G_RequestMap("START");

  x32 then = 0;
  x32 time = Sys_FixedTime();
  x32 dt;
  while (1) {
    then = time;
    time = Sys_FixedTime();
    dt = time - then;
    gs.frametime = (dt > 0x7FFF) ? 0x7FFF : dt;

    // if the map has changed, reset time
    if (G_CheckNextMap())
      continue;

    Prf_StartFrame();

    IN_Update();
    G_Update(gs.frametime);
    CD_Update();
    Snd_Update(&rs.origin, &rs.vright);
    R_UpdateLightStyles(gs.time);
    R_RenderView();
    R_Flip();

    Prf_EndFrame();
  }

  return 0;
}
