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

  while (1) {
    Sys_UpdateTime();
    gs.frametime = Sys_FixedDeltaTime();

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
