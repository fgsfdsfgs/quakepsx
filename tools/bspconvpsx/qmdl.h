#pragma once

#include "../common/psxtypes.h"
#include "../common/idmdl.h"

typedef struct {
  u8 *start;
  size_t size;
  qaliashdr_t *header;
} qmdl_t;

int qmdl_init(qmdl_t *mdl, u8 *start, const size_t size);
