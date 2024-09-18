#pragma once

#include "../common/psxtypes.h"
#include "../common/idmdl.h"

#define MAX_QMDLS 128
#define MAX_QMDL_NAME 256

typedef struct {
  char name[MAX_QMDL_NAME];
  u8 *start;
  s16 id;
  size_t size;
  qaliashdr_t *header;
  qaliasframe_t *frames;
  qaliastexcoord_t *texcoords;
  qaliastri_t *tris;
  qaliasskin_t *skins;
} qmdl_t;

extern int num_qmdls;
extern qmdl_t qmdls[MAX_QMDLS];

qmdl_t *qmdl_add(const char *name, u8 *start, const size_t size);
qmdl_t *qmdl_find(const char *name);
int qmdl_id_for_name(const char *name);

int qmdl_init(qmdl_t *mdl, u8 *start, const size_t size);
void qmdl_free(qmdl_t *mdl);
