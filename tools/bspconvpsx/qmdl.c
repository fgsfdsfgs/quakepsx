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

typedef f32 (*sort_weight_fn_t)(qvec3_t va[3]);
static qmdl_t *sort;
static int sort_muzzlehack = 0;
static sort_weight_fn_t sort_weight_fn;

static char *qmdl_props = NULL;
static char *qmdl_props_end = NULL;

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

int qmdlprops_init(const char *propsfile) {
  // fuck it, just load the entire thing and re-parse it every time,
  // I'll hopefully figure out a better config format later
  FILE *f = fopen(propsfile, "rb");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  int sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz > 0) {
    qmdl_props = calloc(1, sz + 1);
    fread(qmdl_props, sz, 1, f);
    qmdl_props_end = qmdl_props + sz;
  } else {
    qmdl_props = calloc(1, 1);
    qmdl_props_end = qmdl_props;
  }
  fclose(f);
  return 1;
}

static int qmdlprops_read(qmdl_t *qmdl) {
  const char *data = strstr(qmdl_props, qmdl->name);
  if (!data) return 0;

  data += strlen(qmdl->name);
  if (!*data) return 0;

  char key[MAX_TOKEN] = { 0 };
  char value[MAX_TOKEN] = { 0 };

  while (1) {
    // parse key
    data = com_parse(data, key);
    if (!data || !key[0])
      break;
    if (!strcmp(key, "viewmodel")) {
      // this model is a viewmodel
      qmdl->viewmodel = 1;
      printf("* * is a viewmodel\n");
    } else if (!strcmp(key, "delete")) {
      // delete a frame or range of frames
      data = com_parse(data, value);
      assert(value[0]);
      if (strstr(value, "..")) {
        int fstart = -1;
        int fend = -1;
        // check if it's a range of frames, otherwise check for a single
        if (sscanf(value, "%d..%d", &fstart, &fend) < 2) {
          sscanf(value, "%d", &fend);
          fstart = fend;
        }
        assert(fstart >= 0 && fend >= 0);
        assert(fend >= fstart);
        assert(fend < qmdl->header->numframes);
        const int ndeleted = fend - fstart + 1;
        const int nright = qmdl->header->numframes - fend - 1;
        if (nright > 0)
          memmove(qmdl->frames + fstart, qmdl->frames + fend + 1, nright * sizeof(qmdl->frames[0]));
        printf("* * deleted frames %d..%d, numframes %d -> %d\n",
          fstart, fend, qmdl->header->numframes, qmdl->header->numframes - ndeleted);
        qmdl->header->numframes -= ndeleted;
      }
    } else if (!strcmp(key, "mdl")) {
      // next model block begins here
      break;
    }
  }

  return 1;
}

qmdl_t *qmdl_find(const char *name) {
  for (s32 i = 0; i < num_qmdls; ++i) {
    if (!strncmp(qmdls[i].name, name, sizeof(qmdls[i].name)))
      return &qmdls[i];
  }

  return NULL;
}

qmdl_t *qmdl_add(const int id, const char *name, u8 *start, const size_t size) {
  assert(num_qmdls < MAX_QMDLS);

  qmdl_t *mdl = &qmdls[num_qmdls++];
  strncpy(mdl->name, name, sizeof(mdl->name) - 1);
  qmdl_init(mdl, start, size);
  mdl->id = id ? id : qmdlmap_id_for_name(name);

  // parse the model's props (this looks through the entire props file every time but whatever)
  qmdlprops_read(mdl);

  // pre-sort viewmodels
  if (mdl->viewmodel)
    qmdl_sort(mdl);

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

  for (s32 i = 0; i < mdl->header->numskins; ++i) {
    mdl->skins[i].group = *(u32 *)start; start += sizeof(u32);
    mdl->skins[i].data = start; start += mdl->header->skinwidth * mdl->header->skinheight;
  }

  mdl->texcoords = (qaliastexcoord_t *)start;
  start += mdl->header->numverts * sizeof(*mdl->texcoords);
  mdl->tris = (qaliastri_t *)start;
  start += mdl->header->numtris * sizeof(*mdl->tris);

  // check if the first frame is a group; we only support models that either have only simple frames or 1 group frame
  qtrivertx_t absmin = { 0, 0, 0, 0 };
  qtrivertx_t absmax = { 0, 0, 0, 0 };
  int isgroup = 0;
  if (*(s32 *)start != 0) {
    // if (mdl->header->numframes != 1)
    //   panic("model has more than one group frame: %d", mdl->header->numframes);
    // this is the only group frame; read the group header and override numframes
    start += sizeof(s32); // skip group frame type
    mdl->header->numframes = *(s32 *)start; start += sizeof(s32);
    absmin = *(qtrivertx_t *)start; start += sizeof(qtrivertx_t);
    absmax = *(qtrivertx_t *)start; start += sizeof(qtrivertx_t);
    start += sizeof(f32) * mdl->header->numframes; // skip time intervals
    isgroup = 1;
  }

  mdl->frames = calloc(mdl->header->numframes, sizeof(*mdl->frames));
  assert(mdl->frames);

  for (s32 i = 0; i < mdl->header->numframes; ++i) {
    if (isgroup) {
      // this frame is part of a group and has no type field
      mdl->frames[i].type = 0;
    } else {
      // hope this is a simple frame
      mdl->frames[i].type = *(s32 *)start; start += sizeof(s32);
      if (mdl->frames[i].type != 0)
        panic("frame %d: expected type 0, got %d\n", i, mdl->frames[i].type);
    }
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

static float tri_weight_avg_x(qvec3_t va[3]) {
  return (va[0][0] + va[1][0] + va[2][0]) / 3.f;
}

static float tri_weight_max_x(qvec3_t va[3]) {
  f32 mx = va[0][0];
  if (va[1][0] > mx)
    mx = va[1][0];
  if (va[2][0] > mx)
    mx = va[2][0];
  return mx;
}

static float tri_weight_min_x(qvec3_t va[3]) {
  f32 mx = va[0][0];
  if (va[1][0] < mx)
    mx = va[1][0];
  if (va[2][0] < mx)
    mx = va[2][0];
  return mx;
}

static int tri_cmp_x(const qaliastri_t *ta, const qaliastri_t *tb) {
  qvec3_t va[3], vb[3];

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      va[i][j] = (f32)((sort->frames[0].verts + ta->vertex[i])->v[j]) * sort->header->scale[j] + sort->header->translate[j];
      vb[i][j] = (f32)((sort->frames[0].verts + tb->vertex[i])->v[j]) * sort->header->scale[j] + sort->header->translate[j];
    }
  }

  f32 cmp_a = sort_weight_fn(va);
  f32 cmp_b = sort_weight_fn(vb);

  // HACK: penalize the muzzle flash part, it's usually stored behind z = -1 in frame 0
  if (sort_muzzlehack) {
    cmp_a += (va[0][0] < -1.f && va[1][0] < -1.f && va[2][0] < -1.f) * 1000.f;
    cmp_b += (vb[0][0] < -1.f && vb[1][0] < -1.f && vb[2][0] < -1.f) * 1000.f;
  }

  if (cmp_a < cmp_b)
    return +1; // smaller X goes in the back so it renders later
  if (cmp_a > cmp_b)
    return -1; // bigger X goes in the front so it renders first
  return 0;
}

void qmdl_sort(qmdl_t *mdl) {
  sort = mdl;
  // TODO: un-hardcode these
  // if this is enabled, the muzzleflash part of the model will sort to the end
  sort_muzzlehack = !strstr(sort->name, "v_axe") && !strstr(sort->name, "v_light");
  // pick how to sort polys
  if (strstr(sort->name, "v_axe"))
    sort_weight_fn = tri_weight_max_x;
  else if (strstr(sort->name, "v_shot2") || strstr(sort->name, "v_nail"))
    sort_weight_fn = tri_weight_min_x;
  else
    sort_weight_fn = tri_weight_avg_x;
  qsort(mdl->tris, mdl->header->numtris, sizeof(*mdl->tris), (void *)tri_cmp_x);
}
