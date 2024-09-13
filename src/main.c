#include "common.h"
#include "system.h"
#include "render.h"
#include "input.h"
#include "game.h"

static void TestInput(const x16 dt) {
  const int fwd = IN_ButtonHeld(PAD_UP) -  IN_ButtonHeld(PAD_DOWN);
  const int side = IN_ButtonHeld(PAD_RIGHT) -  IN_ButtonHeld(PAD_LEFT);
  const int up = IN_ButtonHeld(PAD_R1) -  IN_ButtonHeld(PAD_L1);
  const int pitch = IN_ButtonHeld(PAD_TRIANGLE) -  IN_ButtonHeld(PAD_CROSS);
  const int yaw = IN_ButtonHeld(PAD_SQUARE) -  IN_ButtonHeld(PAD_CIRCLE);

  player_state_t *plr = &gs.player[0];
  plr->move.x = fwd * PLAYER_ACCELERATION;
  plr->move.y = side * PLAYER_ACCELERATION;
  plr->move.z = up * PLAYER_ACCELERATION;
  plr->anglemove.x = -pitch * PLAYER_LOOK_SPEED;
  plr->anglemove.y = yaw * PLAYER_LOOK_SPEED;

  if (IN_ButtonPressed(PAD_L2))
  {
    if (plr->ent->v.movetype == MOVETYPE_WALK)
      plr->ent->v.movetype = MOVETYPE_NOCLIP;
    else
      plr->ent->v.movetype = MOVETYPE_WALK;
  }
}

int main(int argc, char **argv) {
  Sys_Init();
  Mem_Init();
  R_Init();
  IN_Init();

  Mem_SetMark(MEM_MARK_LO);

  G_StartMap(FS_BASE "\\MAPS\\E1M1.PSB;1");

  x32 then = 0;
  while (1) {
    then = gs.time;
    gs.time = Sys_FixedTime();
    gs.frametime = gs.time - then;
    IN_Update();
    TestInput(gs.frametime);
    G_PlayerMove(gs.frametime);
    R_UpdateLightStyles(gs.time);
    R_RenderView();
    R_Flip();
  }

  return 0;
}
