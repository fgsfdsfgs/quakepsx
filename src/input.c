#include <sys/types.h>
#include <libpad.h>
#include <libapi.h>
#include "common.h"
#include "input.h"

static char pad_buff[2][34];
static PADTYPE *pad;

u16 in_buttons;
u16 in_buttons_prev;
u16 in_buttons_trig;

void IN_Init(void)
{
  InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);
  StartPAD();
  // don't make pad driver acknowledge vblank IRQ
  ChangeClearPAD(0);
  pad = (PADTYPE *)&pad_buff[0][0];
}

void IN_Update(void)
{
  if (pad->stat != 0)
    return;

  in_buttons_prev = in_buttons;
  in_buttons = ~pad->btn;

  in_buttons_trig = ~in_buttons_prev & in_buttons;
}
