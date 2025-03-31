#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/util.h"
#include "../common/pak.h"

#define MAX_RES     255
#define MAX_RESNAME 128

typedef char resname_t[MAX_RESNAME];

static resname_t entlist[MAX_RES] = {
  "worldspawn",
  "player",
};
static int numents = 2;

static resname_t mdllist[MAX_RES] = { "NONE" };
static int nummdls = 1;

static resname_t sfxlist[MAX_RES] = { "NONE" };
static int numsfx = 1;

static void resname_add(resname_t reslist[], int *numres, const char *resname) {
  if (resname[0] == '*')
    return; // bsp model
  assert(*numres < MAX_RES);
  for (int i = 0; i < *numres; ++i) {
    if (!strncmp(resname, reslist[i], MAX_RESNAME))
      return;
  }
  strncpy(reslist[(*numres)++], resname, sizeof(resname_t));
}

static void scan_bsp(const u8 *data, resname_t reslist[], int *resnum, const char *field) {
  const qbsphdr_t *hdr = (const qbsphdr_t *)data;
  assert(hdr->ver == QBSPVERSION);
  const int entlen = hdr->entities.len;
  const char *entstr = (const char *)(data + hdr->entities.ofs);
  const char *ptr = entstr;
  const int fieldlen = strlen(field);
  char resname[MAX_RESNAME];
  
  while (*ptr && ptr < entstr + entlen) {
    ptr = strstr(ptr, field); // "\"classname\""
    if (!ptr) break;
    ptr += fieldlen;
    while (*ptr && isspace(*ptr)) ++ptr;
    ++ptr; // skip quote
    const char *end = ptr;
    while (*end && *end != '"' && !isspace(*end)) ++end;
    memcpy(resname, ptr, (end - ptr));
    resname[end - ptr] = '\0';
    resname_add(reslist, resnum, resname);
    ptr = end;
  }
}

static void do_pack(pak_t *pak) {
  // first, scan all BSPs for used entclasses, models and sounds
  const char *name = pak_findfile(pak, ".bsp", NULL);
  for (; name; name = pak_findfile(pak, ".bsp", name)) {
    u8 *data = pak_readfile(pak, name, NULL);
    assert(data);
    scan_bsp(data, entlist, &numents, "\"classname\"");
    // these don't happen in OG quake, but you never know
    scan_bsp(data, mdllist, &nummdls, "\"model\"");
    scan_bsp(data, sfxlist, &numsfx, "\"sound\"");
    free(data);
  }

  // then do resources individually

  // models
  name = pak_findfile(pak, ".mdl", NULL);
  for (; name; name = pak_findfile(pak, ".mdl", name)) {
    resname_add(mdllist, &nummdls, name);
  }
  // and brush models
  name = pak_findfile(pak, "maps/b_", NULL);
  for (; name; name = pak_findfile(pak, "maps/b_", name)) {
    resname_add(mdllist, &nummdls, name);
  }

  // sounds
  name = pak_findfile(pak, ".wav", NULL);
  for (; name; name = pak_findfile(pak, ".wav", name)) {
    resname_add(sfxlist, &numsfx, name);
  }
}

static const char *entstrupr(const char *str) {
  static char upr[MAX_RESNAME + 1];
  int i;
  for (i = 0; i < MAX_RESNAME && str[i]; ++i)
    upr[i] = toupper(str[i]);
  upr[i] = '\0';
  return upr;
}

static const char *resstrupr(const char *str) {
  static char upr[MAX_RESNAME + 1];
  int i;

  if (strlen(str) > 6 && (!strncmp(str, "progs/", 6) || !strncmp(str, "sound/", 6)))
    str += 6;
  else if (strlen(str) > 5 && !strncmp(str, "maps/", 5))
    str += 5;

  for (i = 0; i < MAX_RESNAME && str[i]; ++i) {
    if (str[i] == '.')
      break;
    else if (str[i] == '/' || str[i] == '\\')
      upr[i] = '_';
    else
      upr[i] = toupper(str[i]);
  }
  upr[i] = '\0';

  return upr;
}

static int do_entclasses(const char *txtname, const char *hname, const char *cname) {
  // sort entclasses by name (except for worldspawn and player)
  qsort(entlist + 2, numents - 2, sizeof(*entlist), (void *)strcmp);

  FILE *fm = fopen(txtname, "wb");
  if (!fm) {
    fprintf(stderr, "could not open '%s'\n", txtname);
    return -2;
  }
  FILE *fh = fopen(hname, "wb");
  if (!fh) {
    fprintf(stderr, "could not open '%s'\n", hname);
    fclose(fm);
    return -3;
  }
  FILE *fc = fopen(cname, "wb");
  if (!fc) {
    fprintf(stderr, "could not open '%s'\n", cname);
    fclose(fm);
    fclose(fh);
    return -4;
  }

  fprintf(fh, "#pragma once\n\n#include \"entity.h\"\n\nenum entclass_e {\n");
  fprintf(fc, "#include \"entity.h\"\n#include \"entclasses.h\"\n\n");

  for (int i = 2; i < numents; ++i)
    fprintf(fc, "extern void spawn_%s(edict_t *self);\n", entlist[i]);
  fprintf(fc, "\nthink_fn_t ent_spawnfuncs[] = {\n");

  for (int i = 0; i < numents; ++i) {
    fprintf(fm, "%02x %s\n", i, entlist[i]);
    fprintf(fh, "  ENT_%s = 0x%02x,\n", entstrupr(entlist[i]), i);
    if (i >= 2) fprintf(fc, "  [ENT_%s] = spawn_%s,\n", entstrupr(entlist[i]), entlist[i]);
  }

  fprintf(fh, "};\n\nextern think_fn_t ent_spawnfuncs[];\n");
  fprintf(fc, "};\n");

  fclose(fm);
  fclose(fh);
  fclose(fc);

  return 0;
}

static int do_res(resname_t reslist[], const int numres, const char *prefix, const char *txtname, const char *hname) {
  // sort res by name
  qsort(reslist, numres, sizeof(resname_t), (void *)strcmp);

  FILE *fm = fopen(txtname, "wb");
  if (!fm) {
    fprintf(stderr, "could not open '%s'\n", txtname);
    return -2;
  }

  FILE *fh = fopen(hname, "wb");
  if (!fh) {
    fprintf(stderr, "could not open '%s'\n", hname);
    fclose(fm);
    return -3;
  }

  fprintf(fh, "#pragma once\n\nenum {\n");

  for (int i = 0; i < numres; ++i) {
    if (i) fprintf(fm, "%02x %s\n", i, reslist[i]);
    fprintf(fh, "  %s%s = 0x%02x,\n", prefix, resstrupr(reslist[i]), i);
  }

  fprintf(fh, "};\n");

  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("usage: %s <moddir> <outdir>\n", argv[0]);
    return -1;
  }

  char pakname[] = "pak0.pak";
  pak_t pak;
  for (int i = '0'; i <= '9'; ++i) {
    pakname[3] = i;
    if (pak_open(&pak, strfmt("%s/%s", argv[1], pakname)) == 0) {
      do_pack(&pak);
      pak_close(&pak);
    }
  }

  const char *txtname = strfmt("%s/entmap.txt", argv[2]);
  const char *hname = strfmt("%s/entclasses.h", argv[2]);
  const char *cname = strfmt("%s/entclasses.c", argv[2]);
  do_entclasses(txtname, hname, cname);

  if (nummdls) {
    txtname = strfmt("%s/mdlmap.txt", argv[2]);
    hname = strfmt("%s/mdlids.h", argv[2]);
    do_res(mdllist, nummdls, "MDLID_", txtname, hname);
  }

  if (numsfx) {
    txtname = strfmt("%s/sfxmap.txt", argv[2]);
    hname = strfmt("%s/sfxids.h", argv[2]);
    do_res(sfxlist, numsfx, "SFXID_", txtname, hname);
  }

  return 0;
}
