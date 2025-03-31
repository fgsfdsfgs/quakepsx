#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "qmdl.h"
#include "util.h"
#include "../common/psxtypes.h"
#include "../common/idmdl.h"

static char qmdlmap[MAX_QMDLS][MAX_QMDL_NAME];
static int num_qmdlmap = 0;

int num_qmdls = 0;
qmdl_t qmdls[MAX_QMDLS];

int qmdlmap_init(const char *mapfile) {
  num_qmdlmap = resmap_parse(mapfile, (char *)qmdlmap, MAX_QMDLS, sizeof(qmdlmap[0]), sizeof(qmdlmap[0]));
  printf("qmdlmap_init(): indexed %d models from %s\n", num_qmdlmap, mapfile);
  return num_qmdlmap;
}

int qmdlmap_id_for_name(const char *name) {
  for (s32 i = 0; i < num_qmdlmap; ++i) {
    if (!strcmp(name, qmdlmap[i])) {
      return i;
      break;
    }
  }
  return 0;
}

const char *qmdlmap_name_for_id(const int id) {
  if (id <= 0 || id >= num_qmdlmap)
    return NULL;
  return qmdlmap[id];
}

qmdl_t *qmdl_find(const char *name) {
  for (s32 i = 0; i < num_qmdls; ++i) {
    if (!strncmp(qmdls[i].name, name, sizeof(qmdls[i].name)))
      return &qmdls[i];
  }

  return NULL;
}

qmdl_t *qmdl_add(const char *name, u8 *start, const size_t size) {
  assert(num_qmdls < MAX_QMDLS);

  qmdl_t *mdl = &qmdls[num_qmdls++];
  strncpy(mdl->name, name, sizeof(mdl->name) - 1);
  qmdl_init(mdl, start, size);
  mdl->id = qmdlmap_id_for_name(name);

  return mdl;
}

int qmdl_init(qmdl_t *mdl, u8 *start, const size_t size) {
  mdl->start = start;
  mdl->size = size;

  mdl->header = (qaliashdr_t *)start;
  if (mdl->header->version != ALIAS_VERSION)
    panic("malformed Quake MDL: expected version %d, got %d\n", ALIAS_VERSION, mdl->header->version);

  start += sizeof(qaliashdr_t);

  mdl->skins = calloc(mdl->header->numskins, sizeof(*mdl->skins));
  assert(mdl->skins);

  mdl->frames = calloc(mdl->header->numframes, sizeof(*mdl->frames));
  assert(mdl->frames);

  for (s32 i = 0; i < mdl->header->numskins; ++i) {
    mdl->skins[i].group = *(u32 *)start; start += sizeof(u32);
    mdl->skins[i].data = start; start += mdl->header->skinwidth * mdl->header->skinheight;
  }

  mdl->texcoords = (qaliastexcoord_t *)start;
  start += mdl->header->numverts * sizeof(*mdl->texcoords);
  mdl->tris = (qaliastri_t *)start;
  start += mdl->header->numtris * sizeof(*mdl->tris);

  for (s32 i = 0; i < mdl->header->numframes; ++i) {
    mdl->frames[i].type = *(s32 *)start; start += sizeof(s32);
    if (mdl->frames[i].type != 0)
      panic("frame %d: expected type 0, got %d\n", i, mdl->frames[i].type);
    mdl->frames[i].min = *(qtrivertx_t *)start; start += sizeof(qtrivertx_t);
    mdl->frames[i].max = *(qtrivertx_t *)start; start += sizeof(qtrivertx_t);
    memcpy(mdl->frames[i].name, start, sizeof(mdl->frames[i].name));
    start += sizeof(mdl->frames[i].name);
    mdl->frames[i].verts = (qtrivertx_t *)start;
    start += sizeof(mdl->frames[i].verts[0]) * mdl->header->numverts;
  }

  return 0;
}

void qmdl_free(qmdl_t *mdl) {
  free(mdl->skins);
  mdl->skins = NULL;
  free(mdl->frames);
  mdl->frames = NULL;
}
