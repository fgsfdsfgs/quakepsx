#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/psxbsp.h"
#include "../common/util.h"
#include "../common/pak.h"

#include "util.h"
#include "xbsp.h"
#include "xmdl.h"
#include "qbsp.h"
#include "qmdl.h"
#include "qsfx.h"
#include "qent.h"

static const char *cfgdir;
static const char *moddir;
static const char *inname;
static const char *outname;
static const char *vramexp;
static int print_targetnames = 0;

static int qbsp_worldtype = 0;
static int qbsp_cdtrack = 0;

static void cleanup(void) {
  if (qbsp.start) free(qbsp.start);
  if (qbsp.palette) free(qbsp.palette);
}

struct texsort { const qmiptex_t *qtex; int id; };

static int tex_sort(const struct texsort *a, const struct texsort *b) {
  if (a->qtex == NULL) {
    return 1;
  }
  if (b->qtex == NULL) {
    return -1;
  }

  int aw = a->qtex->width;
  int ah = a->qtex->height;
  xbsp_texture_shrink(&aw, &ah, MAX_TEX_WIDTH, MAX_TEX_HEIGHT);
  int bw = b->qtex->width;
  int bh = b->qtex->height;
  xbsp_texture_shrink(&bw, &bh, MAX_TEX_WIDTH, MAX_TEX_HEIGHT);

  if (ah > bh) {
    return -1;
  } else if (ah == bh) {
    if (bw == aw)
      return a->id - b->id;
    else
      return bw - aw;
  } else {
    return 1;
  }
}

static inline void do_texture_anims(void) {
  const int numtex = qbsp.miptex->nummiptex;
  xtexinfo_t *anims[10];
  xtexinfo_t *altanims[10];
  int num, max, altmax;
  int num_animtex = 0;

  for (int i = 0; i < numtex; ++i) {
    const qmiptex_t *qtex = qbsp_get_miptex(&qbsp, i);
    xtexinfo_t *xti = xbsp_texinfos + i;

    if (!qtex || qtex->name[0] != '+')
      continue;
    if (xti->anim_next >= 0)
      continue; // already sequenced

    ++num_animtex;

    memset(anims, 0, sizeof(anims));
    memset(altanims, 0, sizeof(altanims));
    max = qtex->name[1];
    altmax = 0;

    if (max >= 'a' && max <= 'z')
      max -= 'a' - 'A';
    if (max >= '0' && max <= '9') {
      max -= '0';
      altmax = 0;
      anims[max] = xti;
      max++;
    } else if (max >= 'A' && max <= 'J') {
      altmax = max - 'A';
      max = 0;
      altanims[altmax] = xti;
      altmax++;
    } else {
      panic("bad animating texture %s", qtex->name);
    }

    for (int j = i + 1; j < numtex; ++j) {
      const qmiptex_t *qtex2 = qbsp_get_miptex(&qbsp, j);
      xtexinfo_t *xti2 = xbsp_texinfos + j;
      if (!qtex2 || qtex2->name[0] != '+')
        continue;
      if (strcmp(qtex2->name + 2, qtex->name + 2))
        continue;
      num = qtex2->name[1];
      if (num >= 'a' && num <= 'z')
        num -= 'a' - 'A';
      if (num >= '0' && num <= '9') {
        num -= '0';
        anims[num] = xti2;
        if (num + 1 > max)
          max = num + 1;
      } else if (num >= 'A' && num <= 'J') {
        num = num - 'A';
        altanims[num] = xti2;
        if (num + 1 > altmax)
          altmax = num + 1;
      } else {
        panic("bad animating texture %s", qtex->name);
      }
    }

    // link them all together
    const int anim_cycle = 2;
    for (int j = 0; j < max; ++j) {
      xtexinfo_t *xti2 = anims[j];
      assert(xti2);
      xti2->anim_total = max * anim_cycle;
      xti2->anim_min = j * anim_cycle;
      xti2->anim_max = (j + 1) * anim_cycle;
      xti2->anim_next = anims[(j + 1) % max] - xbsp_texinfos;
      if (altmax)
        xti2->anim_alt = altanims[0] - xbsp_texinfos;
    }
    for (int j = 0; j < altmax; ++j) {
      xtexinfo_t *xti2 = altanims[j];
      assert(xti2);
      xti2->anim_total = altmax * anim_cycle;
      xti2->anim_min = j * anim_cycle;
      xti2->anim_max = (j + 1) * anim_cycle;
      xti2->anim_next = altanims[(j + 1) % altmax] - xbsp_texinfos;
      if (max)
        xti2->anim_alt = anims[0] - xbsp_texinfos;
    }
  }

  printf("* sequenced %d animated textures\n", num_animtex);
}

static inline void do_textures(void) {
  const int numtex = qbsp.miptex->nummiptex;

  if (numtex > MAX_XMAP_TEXTURES)
    panic("input map has too many textures (%d > %d)", numtex, MAX_XMAP_TEXTURES);

  // sort textures by size, largest first
  struct texsort sorted[numtex];
  for (int i = 0; i < numtex; ++i) {
    sorted[i] = (struct texsort) {
      .qtex = qbsp_get_miptex(&qbsp, i),
      .id = i
    };
  }
  qsort(sorted, numtex, sizeof(struct texsort), (void *)tex_sort);

  printf("converting %d textures:\n", numtex);

  for (int i = 0; i < numtex; ++i) {
    const qmiptex_t *qtex = sorted[i].qtex;
    xtexinfo_t xti = { .flags = 0 };
    xti.flags |= xbsp_texture_flags(qtex);
    if ((xti.flags & XTEX_INVISIBLE) == 0) {
      printf("* %03d '%s' (%dx%d)\n", sorted[i].id, qtex->name, qtex->width, qtex->height);
      int vrx = 0;
      int vry = 0;
      if (xbsp_vram_fit(qtex, &xti, &vrx, &vry))
        panic("VRAM atlas can't fit '%s'", qtex->name);
      xbsp_vram_store_miptex(qtex, &xti, vrx, vry);
      // clear animation data
      xti.anim_alt = -1;
      xti.anim_next = -1;
      xti.anim_min = 0;
      xti.anim_max = 0;
      xti.anim_total = 0;
    }
    // store texinfo in normal order
    xbsp_texinfos[sorted[i].id] = xti;
  }

  // sequence the animations
  do_texture_anims();

  xbsp_numtexinfos = numtex;
  xbsp_lumps[XLMP_TEXDATA].size = 2 * VRAM_TOTAL_WIDTH * xbsp_vram_height();
  xbsp_lumps[XLMP_TEXINFO].size = numtex * sizeof(xtexinfo_t);
}

static void do_planes(void) {
  printf("converting %d planes\n", qbsp.numplanes);

  for (int i = 0; i < qbsp.numplanes; ++i) {
    xplane_t *xp = xbsp_planes + i;
    xp->type = qbsp.planes[i].type;
    xp->dist = f32_to_x32(qbsp.planes[i].dist * XBSP_SCALE);
    xp->normal = qvec3_to_x16vec3(qbsp.planes[i].normal);
  }

  xbsp_numplanes = qbsp.numplanes;
  xbsp_lumps[XLMP_PLANES].size = sizeof(xplane_t) * qbsp.numplanes;
}

static void do_faces(void) {
  printf("converting %d faces\n", qbsp.numfaces);

  for (int f = 0; f < qbsp.numfaces; ++f) {
    const qface_t *qf = qbsp.faces + f;
    xface_t *xf = xbsp_faces + f;
    xbsp_face_add(xf, qf, &qbsp);
    // printf("* qface %05d numverts %03d -> %03d\n", f, qf->numedges, xf->numverts);
  }

  xbsp_numfaces = qbsp.numfaces;

  xbsp_lumps[XLMP_FACES].size = xbsp_numfaces * sizeof(xface_t);
  xbsp_lumps[XLMP_VERTS].size = xbsp_numverts * sizeof(xvert_t);

  printf("* added %d/%d vertices in %d faces\n", xbsp_numverts, MAX_XMAP_VERTS, xbsp_numfaces);
}

static void do_visdata(void) {
  xbsp_numvisdata = xbsp_lumps[XLMP_VISILIST].size = qbsp.numvisdata;
  memcpy(xbsp_visdata, qbsp.visdata, qbsp.numvisdata);
  printf("visdata size: %d\n", qbsp.numvisdata);
}

static void do_leaf_light(xleaf_t *xl) {
  // default to fully dark
  xl->styles[0] = MAX_LIGHTSTYLES;
  xl->styles[1] = MAX_LIGHTSTYLES;
  xl->lightmap[0] = 0;
  xl->lightmap[1] = 0;

  // pick the brightest surface in leaf
  u32 max_light = 0;
  const xface_t *max_light_xf = NULL;
  for (int i = 0; i < xl->nummarksurfaces; ++i) {
    const xface_t *xf = xbsp_faces + xbsp_marksurfs[xl->firstmarksurface + i];
    u32 light = 0;
    for (int j = 0; j < xf->numverts; ++j) {
      const xvert_t *xv = xbsp_verts + xf->firstvert + j;
      light += xv->col[0];
      light += xv->col[1];
    }
    light /= xf->numverts;
    if (light > max_light) {
      max_light = light;
      max_light_xf = xf;
    }
  }

  if (!max_light_xf)
    return;

  u32 avglight[MAX_XMAP_LIGHTVALS] = { 0, 0 };
  for (int j = 0; j < max_light_xf->numverts; ++j) {
    const xvert_t *xv = xbsp_verts + max_light_xf->firstvert + j;
    avglight[0] += xv->col[0];
    avglight[1] += xv->col[1];
  }
  avglight[0] /= max_light_xf->numverts;
  avglight[1] /= max_light_xf->numverts;
  if (avglight[0] > 0xFF) avglight[0] = 0xFF;
  if (avglight[1] > 0xFF) avglight[1] = 0xFF;

  xl->styles[0] = max_light_xf->styles[0];
  xl->styles[1] = max_light_xf->styles[1];
  xl->lightmap[0] = avglight[0];
  xl->lightmap[1] = avglight[1];
}

static void do_nodes(void) {
  printf("converting %d nodes, %d clipnodes, %d leaves, %d marksurfaces\n",
    qbsp.numnodes, qbsp.numcnodes, qbsp.numleafs, qbsp.nummarksurf);

  for (int i = 0; i < qbsp.numnodes; ++i) {
    const qnode_t *qn = qbsp.nodes + i;
    xnode_t *xn = xbsp_nodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->firstface = qn->firstface;
    xn->numfaces = qn->numfaces;
    xn->planenum = qn->planenum;
    xn->mins.d[0] = qn->mins[0] * XBSP_SCALE;
    xn->mins.d[1] = qn->mins[1] * XBSP_SCALE;
    xn->mins.d[2] = qn->mins[2] * XBSP_SCALE;
    xn->maxs.d[0] = qn->maxs[0] * XBSP_SCALE;
    xn->maxs.d[1] = qn->maxs[1] * XBSP_SCALE;
    xn->maxs.d[2] = qn->maxs[2] * XBSP_SCALE;
  }

  for (int i = 0; i < qbsp.numcnodes; ++i) {
    const qclipnode_t *qn = qbsp.cnodes + i;
    xclipnode_t *xn = xbsp_clipnodes + i;
    xn->children[0] = qn->children[0];
    xn->children[1] = qn->children[1];
    xn->planenum = qn->planenum;
  }

  for (int i = 0; i < qbsp.numleafs; ++i) {
    const qleaf_t *ql = qbsp.leafs + i;
    xleaf_t *xl = xbsp_leafs + i;
    xl->contents = ql->contents;
    xl->visofs = ql->visofs;
    xl->firstmarksurface = ql->firstmarksurface;
    xl->nummarksurfaces = ql->nummarksurfaces;
    xl->mins.d[0] = ql->mins[0] * XBSP_SCALE;
    xl->mins.d[1] = ql->mins[1] * XBSP_SCALE;
    xl->mins.d[2] = ql->mins[2] * XBSP_SCALE;
    xl->maxs.d[0] = ql->maxs[0] * XBSP_SCALE;
    xl->maxs.d[1] = ql->maxs[1] * XBSP_SCALE;
    xl->maxs.d[2] = ql->maxs[2] * XBSP_SCALE;
  }

  memcpy(xbsp_marksurfs, qbsp.marksurf, qbsp.nummarksurf * 2);

  for (int i = 0; i < qbsp.numleafs; ++i)
    do_leaf_light(xbsp_leafs + i);

  xbsp_nummarksurfs = qbsp.nummarksurf;
  xbsp_numnodes = qbsp.numnodes;
  xbsp_numclipnodes = qbsp.numcnodes;
  xbsp_numleafs = qbsp.numleafs;
  xbsp_lumps[XLMP_NODES].size = qbsp.numnodes * sizeof(xnode_t);
  xbsp_lumps[XLMP_CLIPNODES].size = qbsp.numcnodes * sizeof(xclipnode_t);
  xbsp_lumps[XLMP_LEAFS].size = qbsp.numleafs * sizeof(xleaf_t);
  xbsp_lumps[XLMP_MARKSURF].size = qbsp.nummarksurf * 2;
}

static void do_models(void) {
  printf("converting %d BSP models\n", qbsp.nummodels);

  for (int i = 0; i < qbsp.nummodels; ++i) {
    const qmodel_t *qm = qbsp.models + i;
    xmodel_t *xm = xbsp_models + i;
    xm->firstface = qm->firstface;
    xm->numfaces = qm->numfaces;
    xm->visleafs = qm->visleafs;
    xm->headnode[0] = qm->headnode[0];
    xm->headnode[1] = qm->headnode[1];
    xm->headnode[2] = qm->headnode[2];
    xm->headnode[3] = qm->headnode[3];
    xm->mins.d[0] = qm->mins[0] * XBSP_SCALE;
    xm->mins.d[1] = qm->mins[1] * XBSP_SCALE;
    xm->mins.d[2] = qm->mins[2] * XBSP_SCALE;
    xm->maxs.d[0] = qm->maxs[0] * XBSP_SCALE;
    xm->maxs.d[1] = qm->maxs[1] * XBSP_SCALE;
    xm->maxs.d[2] = qm->maxs[2] * XBSP_SCALE;
  }

  xbsp_nummodels = qbsp.nummodels;
  xbsp_lumps[XLMP_MODELS].size = qbsp.nummodels * sizeof(xmodel_t);
}

static void do_entities(void) {
  printf("parsing entities\n");

  qent_load(qbsp.start + qbsp.header->entities.ofs, qbsp.header->entities.len);

  printf("loaded %d entities, converting...\n", num_qents);

  assert(num_qents > 0);

  qent_t *qent = qents;
  xmapent_t *xent = xbsp_entities;
  const char *tmpstr;
  qvec3_t tmpvec;
  float tmpfloat;
  int tmpint;

  // 0 - worldspawn
  assert(!strcmp(qent->classname, "worldspawn"));
  xent->classname = 0;
  // remember these two, we'll use them later
  if (qent_get_int(qent, "sounds", &tmpint))
    xent->noise = qbsp_cdtrack = tmpint;
  if (qent_get_int(qent, "worldtype", &tmpint))
    qbsp_worldtype = tmpint;
  ++xent; ++qent;
  ++xbsp_numentities;

  // 1 - player spawn, we'll fill it in later
  xent->classname = 1;
  ++xbsp_numentities;

  // the rest
  for (int i = 1; i < num_qents; ++i, ++qent) {
    // filter some entities that we don't need
    if (!strcmp(qent->classname, "info_player_deathmatch") ||
        !strcmp(qent->classname, "info_player_coop") ||
        !strcmp(qent->classname, "info_null"))
      continue;

    // don't care about lights that don't get triggered and have no model
    const int is_light = !strncmp(qent->classname, "light", 5);
    if (is_light &&
        !qent_get_string(qent, "targetname") &&
        !qent_get_string(qent, "model") &&
        !qent->info->mdlnums[0][0])
      continue;

    // if this is a player start, fill in entity 1; otherwise alloc a new one
    if (!strcmp(qent->classname, "info_player_start")) {
      xent = &xbsp_entities[1];
    } else {
      assert(xbsp_numentities < MAX_XMAP_ENTITIES);
      xent = &xbsp_entities[xbsp_numentities++];
      xent->classname = qent->info->classnum;
    }

    // origin
    if (qent_get_vector(qent, "origin", tmpvec))
      xent->origin = qvec3_to_x32vec3(tmpvec);

    // angles
    if (qent_get_vector(qent, "angles", tmpvec)) {
      xent->angles = qvec3_to_x16vec3_angles(tmpvec);
    } else if (qent_get_vector(qent, "mangle", tmpvec)) {
      // used by info_intermission
      xent->angles = qvec3_to_x16vec3_angles(tmpvec);
    } else if (qent_get_float(qent, "angle", &tmpfloat)) {
      // -1 and -2 are special angles for up and down
      if (tmpfloat == -1.f || tmpfloat == -2.f)
        xent->angles.y = (s32)tmpfloat;
      else
        xent->angles.y = f32_to_x16deg(tmpfloat);
    }

    // model: models beginning with * are brush models
    xent->model = 0;
    tmpstr = qent_get_string(qent, "model");
    if (tmpstr) {
      if (tmpstr[0] == '*') {
        xent->model = -atoi(tmpstr + 1);
      } else {
        xent->model = qmdlmap_id_for_name(tmpstr);
      }
    }

    // string fields

    tmpstr = qent_get_string(qent, "message");
    if (tmpstr)
      xent->string = xbsp_string_add(tmpstr);

    tmpstr = qent_get_string(qent, "map");
    if (tmpstr)
      xent->string = xbsp_string_add_upper(tmpstr);

    tmpstr = qent_get_string(qent, "target");
    if (tmpstr)
      xent->target = xbsp_targetname_id(tmpstr);

    tmpstr = qent_get_string(qent, "killtarget");
    if (tmpstr)
      xent->killtarget = xbsp_targetname_id(tmpstr);

    tmpstr = qent_get_string(qent, "targetname");
    if (tmpstr)
      xent->targetname = xbsp_targetname_id(tmpstr);

    // other fields

    if (qent_get_int(qent, "spawnflags", &tmpint))
      xent->spawnflags = tmpint;

    if (qent_get_int(qent, "sounds", &tmpint))
      xent->noise = tmpint;

    if (qent_get_int(qent, "count", &tmpint))
      xent->count = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "lip", &tmpint))
      xent->count = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "style", &tmpint) && is_light)
      xent->count = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "dmg", &tmpint))
      xent->dmg = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "speed", &tmpint))
      xent->speed = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "height", &tmpint))
      xent->height = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_int(qent, "health", &tmpint))
      xent->health = (tmpint > 0x7FFF) ? 0x7FFF : tmpint;

    if (qent_get_float(qent, "wait", &tmpfloat))
      xent->wait = f32_to_x32(tmpfloat);

    if (qent_get_float(qent, "delay", &tmpfloat))
      xent->delay = f32_to_x32(tmpfloat);
  }

  xbsp_lumps[XLMP_ENTITIES].size = sizeof(*xent) * xbsp_numentities;
  xbsp_lumps[XLMP_STRINGS].size = xbsp_numstrings;
  printf("converted to %d entities, entity lump size = %d, string lump size = %d\n",
    xbsp_numentities, xbsp_lumps[XLMP_ENTITIES].size, xbsp_lumps[XLMP_STRINGS].size);
}

static void load_entmodel(const int id, int override_id) {
  if (xbsp_entmodel_find(id))
    return;

  if (!override_id)
    override_id = id;

  const char *mdlname = qmdlmap_name_for_id(override_id);

  printf("* loading model %s (id %02x)\n", mdlname, override_id);

  size_t mdlsize = 0;
  u8 *mdl = lmp_read(moddir, mdlname, &mdlsize);
  if (!mdl) {
    printf("* * not found!\n");
  } else if (strstr(mdlname, ".bsp")) {
    static qbsp_t tmpqbsp;
    qbsp_init(&tmpqbsp, mdl, mdlsize);
    xaliashdr_t *xmdl = xbsp_entmodel_add_from_qbsp(&tmpqbsp, id);
    xmdl->id = id;
  } else {
    qmdl_t *qmdl = qmdl_add(id, mdlname, mdl, mdlsize);
    xbsp_entmodel_add_from_qmdl(qmdl);
  }
}

static void do_entmodels(void) {
  printf("adding entity models\n");

  for (int i = 0; i < num_qents; ++i) {
    // add any models tied to the classname
    for (int j = 0; j < MAX_ENT_MDLS && qents[i].info->mdlnums[j][0]; ++j) {
      // override actual model with worldtype alternative, if there is one
      const int main_id = qents[i].info->mdlnums[j][0];
      const int override_id = qents[i].info->mdlnums[j][qbsp_worldtype];
      load_entmodel(main_id, override_id);
    }
    // also add any extra models
    const char *model = qent_get_string(&qents[i], "model");
    if (model && model[0] != '*' && model[0]) {
      const int id = qmdlmap_id_for_name(model);
      if (id > 0)
        load_entmodel(id, id);
      else
        fprintf(stderr, "* unknown model '%s' in entity %d's model field\n", model, i);
    }
  }

  xbsp_lumps[XLMP_TEXDATA].size = 2 * VRAM_TOTAL_WIDTH * xbsp_vram_height();
  xbsp_lumps[XLMP_MDLDATA].size = sizeof(xmdllump_t) + sizeof(xaliashdr_t) * xbsp_numentmodels + (xbsp_entmodeldataptr - xbsp_entmodeldata);
  printf("loaded %d entity models, mdl lump size = %u\n", num_qmdls, xbsp_lumps[XLMP_MDLDATA].size);
}

static void load_sound(const int id, int override_id, const int allow_ambient) {
  if (qsfx_find(id))
    return;

  if (!override_id)
    override_id = id;

  const char *sfxname = qsfxmap_name_for_id(override_id);
  if (!allow_ambient && strstr(sfxname, "ambience"))
    return; // ambient sounds not allowed

  printf("* loading sound %s (id %02x)\n", sfxname, override_id);

  size_t wavsize = 0;
  u8 *wavdata = lmp_read(moddir, sfxname, &wavsize);
  if (!wavdata)
    return;

  qsfx_t *sfx = qsfx_add(id, sfxname, wavdata, wavsize);
  free(wavdata);
  if (!sfx)
    return;

  if (!xbsp_spu_fit(sfx))
    printf("* ! could not fit in SPU RAM\n");
}

static void do_sounds(void) {
  printf("adding entity sounds\n");

  // HACK: can't be arsed to make a sorted list, so do two passes
  // pass 1: all sounds except ambient
  // pass 2: ambient
  for (int ambient = 0; ambient < 2; ++ambient) {
    for (int i = 0; i < num_qents; ++i) {
      // add any sounds tied to the classname
      for (int j = 0; j < MAX_ENT_SFX && qents[i].info->sfxnums[j][0]; ++j) {
        // override actual sound with worldtype alternative, if there is one
        const int main_id = qents[i].info->sfxnums[j][0];
        const int override_id = qents[i].info->sfxnums[j][qbsp_worldtype];
        load_sound(main_id, override_id, ambient);
      }
    }
  }

  xbsp_lumps[XLMP_SNDDATA].size = sizeof(xsndlump_t) + sizeof(xmapsnd_t) * xbsp_numsounds + xbsp_spuptr;
  printf("loaded %d sounds, sfx lump size = %u\n", xbsp_numsounds, xbsp_lumps[XLMP_SNDDATA].size);
}

static inline const char *get_arg(int c, const char **v, const char *arg) {
  for (int i = 4; i < c; ++i) {
    if (!strcmp(v[i], arg)) {
      if (i == c - 1)
        return "";
      else
        return v[i + 1];
    }
  }
  return NULL;
}

int main(int argc, const char **argv) {
  if (argc < 5) {
    printf("usage: %s <cfgdir> <moddir> <mapname> <outxbsp> [<options>]\n", argv[0]);
    return -1;
  }

  atexit(cleanup);

  cfgdir = argv[1];
  moddir = argv[2];
  inname = argv[3];
  outname = argv[4];
  vramexp = get_arg(argc, argv, "--export-vram");
  print_targetnames = get_arg(argc, argv, "--print-targetnames") != NULL;

  // read entity and resource definitions
  assert(qmdlmap_init(strfmt("%s/mdlmap.txt", cfgdir)) >= 0);
  assert(qsfxmap_init(strfmt("%s/sfxmap.txt", cfgdir)) >= 0);
  assert(qentmap_init(strfmt("%s/entmap.txt", cfgdir)) >= 0);
  assert(qentmap_link(strfmt("%s/reslist.txt", cfgdir)) >= 0);

  // read palette
  size_t palsize = 0;
  qbsp.palette = lmp_read(moddir, "gfx/palette.lmp", &palsize);
  if (!qbsp.palette)
    panic("could not load gfx/palette.lmp");
  if (palsize < (NUM_CLUT_COLORS * 3))
    panic("palette size < %d", NUM_CLUT_COLORS * 3);

  // load input map
  size_t bspsize = 0;
  u8 *bsp = lmp_read(moddir, inname, &bspsize);
  if (!bsp)
    panic("could not load '%s'", inname);
  qbsp_init(&qbsp, bsp, bspsize);

  // prepare lump headers
  for (int i = 0; i < XLMP_COUNT; ++i)
    xbsp_lumps[i].type = i;

  do_textures();
  do_planes();
  do_faces();
  do_visdata();
  do_nodes();
  do_models();
  do_entities();
  do_entmodels();
  do_sounds();

  if (vramexp && *vramexp)
    xbsp_vram_export(vramexp, qbsp.palette);

  if (print_targetnames)
    xbsp_targetname_print();

  if (xbsp_write(outname) != 0)
    panic("could not write PSX BSP to '%s'", outname);

  return 0;
}
