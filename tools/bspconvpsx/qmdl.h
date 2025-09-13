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
  s32 viewmodel;
  qaliashdr_t *header;
  qaliasframe_t *frames;
  qaliastexcoord_t *texcoords;
  qaliastri_t *tris;
  qaliasskin_t *skins;
} qmdl_t;

extern int num_qmdls;
extern qmdl_t qmdls[MAX_QMDLS];

int qmdlmap_init(const char *mapfile);
int qmdlmap_id_for_name(const char *name);
const char *qmdlmap_name_for_id(const int id);

int qmdlprops_init(const char *propsfile);

qmdl_t *qmdl_add(const int id, const char *name, u8 *start, const size_t size);
qmdl_t *qmdl_find(const char *name);
void qmdl_sort(qmdl_t *mdl);

int qmdl_init(qmdl_t *mdl, u8 *start, const size_t size);
void qmdl_free(qmdl_t *mdl);
