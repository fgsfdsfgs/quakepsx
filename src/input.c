#include <sys/types.h>
#include <psxpad.h>
#include <psxapi.h>
#include "common.h"
#include "input.h"

input_t in;

static u8 pad_buff[2][34];
static PADTYPE *pad[2];

void IN_Init(void) {
  InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
  StartPAD();

  // don't make pad driver acknowledge vblank IRQ
  ChangeClearPAD(0);
  pad[0] = (PADTYPE *)&pad_buff[0][0];
  pad[1] = (PADTYPE *)&pad_buff[1][0];

  // default sensitivity
  in.stick_sens[0].x = TO_FIX32(16);
  in.stick_sens[0].y = TO_FIX32(16);
  in.stick_sens[1].x = TO_FIX32(16);
  in.stick_sens[1].y = TO_FIX32(8);
  in.mouse_sens = TO_FIX32(32);

  // default deadzone
  in.stick_deadzone[0] = 16;
  in.stick_deadzone[1] = 16;
}

static inline void IN_UpdateAnalog(const PADTYPE *pad) {
  const u8 sx[2] = { pad->ls_x, pad->rs_x };
  const u8 sy[2] = { pad->ls_y, pad->rs_y };

  for (int i = 0; i < 2; ++i) {
    const s16 x = sx[i] - 0x80;
    const s16 y = sy[i] - 0x80;
    const s16 dead = in.stick_deadzone[i];
    if ((x * x + y * y) < dead * dead) {
      in.sticks[i].x = 0;
      in.sticks[i].y = 0;
    } else {
      in.sticks[i].x = x;
      in.sticks[i].y = y;
    }
  }
}

static inline void IN_UpdateMouse(const PADTYPE *pad) {
  in.mouse.x = pad->x_mov;
  in.mouse.y = pad->y_mov;

  // HACK: for now translate mouse buttons to L2/R2
  const u16 btn = ~pad->btn;
  if (btn & PAD_R1)
    in.btn |= PAD_R2;
  if (btn & PAD_L1)
    in.btn |= PAD_L2;
}

void IN_Update(void) {
  if (pad[0]->stat != 0)
    return;

  in.btn_prev = in.btn;
  in.btn = ~pad[0]->btn;

  // if port 1 is an analog pad, update the sticks
  if (pad[0]->type == PAD_ID_ANALOG)
    IN_UpdateAnalog(pad[0]);

  // if port 2 is a mouse, update the mouse deltas
  if (pad[1]->stat == 0 && pad[1]->type == PAD_ID_MOUSE)
    IN_UpdateMouse(pad[1]);

  in.btn_trig = ~in.btn_prev & in.btn;
}

void IN_Clear(void) {
  in.btn_trig = 0;
}
