#pragma once

#include "common.h"

enum pad_buttons {
  PAD_SELECT = 1,
  PAD_L3 = 2,
  PAD_R3 = 4,
  PAD_START = 8,
  PAD_UP = 16,
  PAD_RIGHT = 32,
  PAD_DOWN = 64,
  PAD_LEFT = 128,
  PAD_L2 = 256,
  PAD_R2 = 512,
  PAD_L1 = 1024,
  PAD_R1 = 2048,
  PAD_TRIANGLE = 4096,
  PAD_CIRCLE = 8192,
  PAD_CROSS = 16384,
  PAD_SQUARE = 32768,
};

typedef struct {
  u8  stat;       // Status
  u8  len : 4;    // Data length (in halfwords)
  u8  type : 4;   // Device type
  u16 btn;        // Button states
  u8  rs_x, rs_y; // Right stick coordinates
  u8  ls_x, ls_y; // Left stick coordinates
} PADTYPE;

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
