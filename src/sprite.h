#pragma once

#include "common.h"
#include "progs/picids.h"

#define CONCHARS_SIZE 128

typedef struct {
  u16 tpage;
  u8vec2_t uv;
  u8vec2_t size;
} pic_t;

void Spr_Init(void);
const pic_t *Spr_GetPic(const int id);
