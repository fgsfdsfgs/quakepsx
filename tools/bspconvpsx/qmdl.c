#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "qmdl.h"
#include "util.h"
#include "../common/psxtypes.h"
#include "../common/idmdl.h"

// TODO: move these into an external file or some shit
static const struct {
  s16 id;
  const char *model;
} qmdlmap[] = {
  { 0x01, "progs/armor.mdl" },
  { 0x02, "progs/backpack.mdl" },
  { 0x03, "progs/bolt.mdl" },
  { 0x04, "progs/bolt2.mdl" },
  { 0x05, "progs/bolt3.mdl" },
  { 0x06, "progs/boss.mdl" },
  { 0x07, "progs/demon.mdl" },
  { 0x08, "progs/dog.mdl" },
  { 0x09, "progs/end1.mdl" },
  { 0x0a, "progs/eyes.mdl" },
  { 0x0b, "progs/flame.mdl" },
  { 0x0c, "progs/flame2.mdl" },
  { 0x0d, "progs/g_light.mdl" },
  { 0x0e, "progs/g_nail.mdl" },
  { 0x0f, "progs/g_nail2.mdl" },
  { 0x10, "progs/g_rock.mdl" },
  { 0x11, "progs/g_rock2.mdl" },
  { 0x12, "progs/g_shot.mdl" },
  { 0x13, "progs/gib1.mdl" },
  { 0x14, "progs/grenade.mdl" },
  { 0x15, "progs/invisibl.mdl" },
  { 0x16, "progs/invulner.mdl" },
  { 0x17, "progs/knight.mdl" },
  { 0x18, "progs/lavaball.mdl" },
  { 0x19, "progs/m_g_key.mdl" },
  { 0x1a, "progs/m_s_key.mdl" },
  { 0x1b, "progs/missile.mdl" },
  { 0x1c, "progs/ogre.mdl" },
  { 0x1d, "progs/player.mdl" },
  { 0x1e, "progs/quaddama.mdl" },
  { 0x1f, "progs/s_light.mdl" },
  { 0x20, "progs/s_spike.mdl" },
  { 0x21, "progs/shambler.mdl" },
  { 0x22, "progs/soldier.mdl" },
  { 0x23, "progs/spike.mdl" },
  { 0x24, "progs/suit.mdl" },
  { 0x25, "progs/v_axe.mdl" },
  { 0x26, "progs/v_light.mdl" },
  { 0x27, "progs/v_nail.mdl" },
  { 0x28, "progs/v_nail2.mdl" },
  { 0x29, "progs/v_rock.mdl" },
  { 0x2a, "progs/v_rock2.mdl" },
  { 0x2b, "progs/v_shot.mdl" },
  { 0x2c, "progs/v_shot2.mdl" },
  { 0x2d, "progs/w_g_key.mdl" },
  { 0x2e, "progs/w_s_key.mdl" },
  { 0x2f, "progs/w_spike.mdl" },
  { 0x30, "progs/wizard.mdl" },
  { 0x31, "progs/zom_gib.mdl" },
  { 0x32, "progs/zombie.mdl" },
  { 0x33, "progs/fish.mdl" },
  { 0x34, "progs/hknight.mdl" },
  { 0x35, "progs/enforcer.mdl" },
  { 0x36, "progs/b_g_key.mdl" },
  { 0x37, "progs/b_s_key.mdl" },
  { 0x38, "progs/k_spike.mdl" },
  { 0x39, "progs/laser.mdl" },
  { 0x3a, "progs/oldone.mdl" },
  { 0x3b, "progs/shalrath.mdl" },
  { 0x3c, "progs/tarbaby.mdl" },
  { 0x3d, "progs/teleport.mdl" },
  { 0x3e, "progs/v_spike.mdl" },
  // these are BSPs, but they will be converted to alias models
  { 0x101, "maps/b_batt0.bsp" },
  { 0x102, "maps/b_batt1.bsp" },
  { 0x103, "maps/b_bh10.bsp" },
  { 0x104, "maps/b_bh25.bsp" },
  { 0x105, "maps/b_bh100.bsp" },
  { 0x106, "maps/b_explob.bsp" },
  { 0x107, "maps/b_exbox2.bsp" },
  { 0x108, "maps/b_nail0.bsp" },
  { 0x109, "maps/b_nail1.bsp" },
  { 0x10a, "maps/b_rock0.bsp" },
  { 0x10b, "maps/b_rock1.bsp" },
  { 0x10c, "maps/b_shell0.bsp" },
  { 0x10d, "maps/b_shell1.bsp" },
};

static const int num_qmdlmap = sizeof(qmdlmap) / sizeof(*qmdlmap);

int num_qmdls = 0;
qmdl_t qmdls[MAX_QMDLS];

qmdl_t *qmdl_find(const char *name) {
  for (s32 i = 0; i < num_qmdls; ++i) {
    if (!strncmp(qmdls[i].name, name, sizeof(qmdls[i].name)))
      return &qmdls[i];
  }

  return NULL;
}

int qmdl_id_for_name(const char *name) {
  for (s32 i = 0; i < num_qmdlmap; ++i) {
    if (!strcmp(name, qmdlmap[i].model)) {
      return qmdlmap[i].id;
      break;
    }
  }
  return 0;
}

qmdl_t *qmdl_add(const char *name, u8 *start, const size_t size) {
  assert(num_qmdls < MAX_QMDLS);

  qmdl_t *mdl = &qmdls[num_qmdls++];
  strncpy(mdl->name, name, sizeof(mdl->name) - 1);
  qmdl_init(mdl, start, size);
  mdl->id = qmdl_id_for_name(name);

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
