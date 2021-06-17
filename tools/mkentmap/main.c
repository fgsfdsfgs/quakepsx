#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/util.h"
#include "../common/pak.h"

#define MAX_ENTCLASSES 255
#define MAX_ENTNAME    64

static char entlist[MAX_ENTCLASSES][MAX_ENTNAME] = {
  "worldspawn",
  "player",
};
static int numents = 2;

static void classname_add(const char *classname) {
  assert(numents < MAX_ENTCLASSES);
  for (int i = 0; i < numents; ++i) {
    if (!strncmp(classname, entlist[i], MAX_ENTNAME))
      return;
  }
  strncpy(entlist[numents++], classname, sizeof(*entlist));
}

static void do_bsp(const u8 *data) {
  const qbsp_t *hdr = (const qbsp_t *)data;
  assert(hdr->ver == QBSPVERSION);
  const int entlen = hdr->entities.len;
  const char *entstr = (const char *)(data + hdr->entities.ofs);
  const char *ptr = entstr;
  char classname[MAX_ENTNAME];
  while (*ptr && ptr < entstr + entlen) {
    ptr = strstr(ptr, "\"classname\"");
    if (!ptr) break;
    ptr += 11;
    while (*ptr && isspace(*ptr)) ++ptr;
    ++ptr; // skip quote
    const char *end = ptr;
    while (*end && *end != '"' && !isspace(*end)) ++end;
    memcpy(classname, ptr, (end - ptr));
    classname[end - ptr] = '\0';
    classname_add(classname);
    ptr = end;
  }
}

static void do_pack(pak_t *pak) {
  const char *name = pak_findfile(pak, ".bsp", NULL);
  for (; name; name = pak_findfile(pak, ".bsp", name)) {
    u8 *data = pak_readfile(pak, name, NULL);
    assert(data);
    do_bsp(data);
    free(data);
  }
}

static const char *entstrupr(const char *str) {
  static char upr[MAX_ENTNAME + 1];
  int i;
  for (i = 0; i < MAX_ENTNAME && str[i]; ++i)
    upr[i] = toupper(str[i]);
  upr[i] = '\0';
  return upr;
}

int main(int argc, char **argv) {
  if (argc != 5) {
    printf("usage: %s <moddir> <entmap> <enthdr> <entc>\n", argv[0]);
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

  // sort entclasses by name (except for worldspawn)
  qsort(entlist + 2, numents - 2, sizeof(*entlist), (void *)strcmp);

  FILE *fm = fopen(argv[2], "wb");
  if (!fm) {
    fprintf(stderr, "could not open '%s'\n", argv[2]);
    return -2;
  }
  FILE *fh = fopen(argv[3], "wb");
  if (!fh) {
    fprintf(stderr, "could not open '%s'\n", argv[3]);
    fclose(fm);
    return -3;
  }
  FILE *fc = fopen(argv[4], "wb");
  if (!fc) {
    fprintf(stderr, "could not open '%s'\n", argv[4]);
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
