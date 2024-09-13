#pragma once

#include <psxpad.h>
#include "common.h"

extern u16 in_buttons;
extern u16 in_buttons_prev;
extern u16 in_buttons_trig;

static inline int IN_ButtonHeld(const u16 btn)
{
  return (in_buttons & btn) != 0;
}

static inline int IN_ButtonPressed(const u16 btn)
{
  return (in_buttons_trig & btn) != 0;
}

void IN_Init(void);
void IN_Update(void);
