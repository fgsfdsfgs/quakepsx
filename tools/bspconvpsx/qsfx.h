#pragma once

#include <stdlib.h>
#include "../common/psxtypes.h"

#define MAX_QSFX 256
#define MAX_QSFX_NAME 256

typedef struct {
  int id;
  int samplerate;
  int looped;
  int numframes;
  int numsamples;
  s16 *samples;
} qsfx_t;

int qsfxmap_init(const char *mapfile);
int qsfxmap_id_for_name(const char *name);
const char *qsfxmap_name_for_id(const int id);

qsfx_t *qsfx_add(const char *name, u8 *start, const size_t size);
qsfx_t *qsfx_find(const int id);
int qsfx_convert(qsfx_t *sfx, u8 *outptr, const int maxlen);
void qsfx_free(qsfx_t *sfx);
