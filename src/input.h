#pragma once

#include <psxpad.h>
#include "common.h"

#define IN_DEADZONE_X 8
#define IN_DEADZONE_Y 8

typedef struct {
  u16 btn;
  u16 btn_prev;
  u16 btn_trig;
  s8vec2_t mouse;
  s8vec2_t sticks[2];
  s32vec2_t stick_sens[2];
  s32 stick_deadzone[2];
  s32 mouse_sens;
} input_t;

extern input_t in;

static inline int IN_ButtonHeld(const u16 btn) {
  return (in.btn & btn) != 0;
}

static inline int IN_ButtonPressed(const u16 btn) {
  return (in.btn_trig & btn) != 0;
}

void IN_Init(void);
void IN_Update(void);
void IN_Clear(void);
