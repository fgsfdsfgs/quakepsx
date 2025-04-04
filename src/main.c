#include "common.h"
#include "system.h"
#include "render.h"
#include "input.h"
#include "game.h"
#include "progs.h"
#include "move.h"
#include "sound.h"

static x32vec3_t sndorg;

static void TestInput(const x16 dt) {
  static int onspeed = 0;

  const int fwd = IN_ButtonHeld(PAD_UP) -  IN_ButtonHeld(PAD_DOWN);
  const int side = IN_ButtonHeld(PAD_RIGHT) -  IN_ButtonHeld(PAD_LEFT);
  const int up = IN_ButtonHeld(PAD_R1) -  IN_ButtonHeld(PAD_L1);
  const int pitch = IN_ButtonHeld(PAD_TRIANGLE) -  IN_ButtonHeld(PAD_CROSS);
  const int yaw = IN_ButtonHeld(PAD_SQUARE) -  IN_ButtonHeld(PAD_CIRCLE);

  player_state_t *plr = &gs.player[0];

  plr->move.x = fwd * G_FORWARDSPEED;
  plr->move.y = side * G_FORWARDSPEED;
  plr->move.z = up * G_FORWARDSPEED;
  plr->anglemove.x = -pitch * G_PITCHSPEED;
  plr->anglemove.y = yaw * G_YAWSPEED;
  plr->movespeed = G_FORWARDSPEED << onspeed;

  if (IN_ButtonPressed(PAD_L3)) {
    if (plr->ent->v.movetype == MOVETYPE_WALK)
      plr->ent->v.movetype = MOVETYPE_NOCLIP;
    else
      plr->ent->v.movetype = MOVETYPE_WALK;
  }

  if (IN_ButtonPressed(PAD_L2))
    onspeed = !onspeed;

  if (IN_ButtonPressed(PAD_SELECT))
    rs.debug = !rs.debug;

  if (IN_ButtonPressed(PAD_START))
    Player_NextWeapon(plr->ent);

  if (IN_ButtonHeld(PAD_R2))
    plr->buttons |= BTN_FIRE;
  else
    plr->buttons &= ~BTN_FIRE;

  if (fwd > 0)
    plr->buttons |= BTN_JUMP;
  else
    plr->buttons &= ~BTN_JUMP;
}

int main(int argc, char **argv) {
  Sys_Init();
  Mem_Init();
  Snd_Init();
  R_Init();
  IN_Init();

  Mem_SetMark(MEM_MARK_LO);

  G_StartMap(FS_BASE "\\MAPS\\E1M2.PSB;1");

  sndorg = gs.edicts[1].v.origin;

  x32 then = 0;
  x32 time = Sys_FixedTime();
  while (1) {
    then = time;
    time = Sys_FixedTime();
    gs.frametime = time - then;
    IN_Update();
    TestInput(gs.frametime);
    G_Update(gs.frametime);
    Snd_Update(&rs.origin, &rs.vright);
    R_UpdateLightStyles(gs.time);
    R_RenderView();
    R_Flip();
  }

  return 0;
}
