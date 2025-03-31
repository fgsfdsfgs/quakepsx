#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "qent.h"
#include "qmdl.h"
#include "qsfx.h"

qentmap_t qentmap[MAX_ENT_CLASSES];
int num_qentmap = 0;

qent_t qents[MAX_ENTITIES];
int num_qents = 0;

qentmap_t *qentmap_find(const char *classname) {
  for (int i = 0; i < num_qentmap; ++i) {
    if (!strcmp(qentmap[i].classname, classname))
      return &qentmap[i];
  }
  return NULL;
}

int qentmap_init(const char *mapfile) {
  num_qentmap = resmap_parse(mapfile, (char *)qentmap, MAX_ENT_CLASSES, sizeof(qentmap[0]), MAX_ENT_CLASSNAME);
  for (int i = 0; i < num_qentmap; ++i) {
    qentmap[i].classnum = i;
  }
  printf("qentmap_init(): indexed %d entclasses from %s\n", num_qentmap, mapfile);
  return num_qentmap;
}

int qentmap_link(const char *resfile) {
  FILE *f = fopen(resfile, "rb");
  if (!f) {
    fprintf(stderr, "qentmap_link(): can't open reslist file %s\n", resfile);
    return -1;
  }

  char line[MAX_TOKEN] = { 0 };
  char classname[MAX_ENT_CLASSNAME] = { 0 };
  qentmap_t *entclass = NULL;
  int id;
  int ret = 0;
  while (fgets(line, sizeof(line), f)) {
    char *key = strtok(line, " \t\r\n");
    if (!key || key[0] == '#')
      continue;

    char *val = strtok(NULL, " \t\r\n");
    if (!key || !val || !key[0] || !val[0])
      continue;

    if (!strcmp(key, "ent")) {
      strncpy(classname, val, sizeof(classname) - 1);
      entclass = qentmap_find(classname);
      if (!entclass) {
        fprintf(stderr, "classname '%s' is not in entmap\n", classname);
        ret = -2;
        break;
      }
    } else {
      if (!entclass) {
        fprintf(stderr, "encountered '%s %s' before any 'ent' directives\n", key, val);
        ret = -3;
        break;
      }
      if (!strcmp(key, "mdl")) {
        assert(entclass->num_mdlnums < MAX_ENT_MDLS);
        id = qmdlmap_id_for_name(val);
        if (id <= 0) {
          fprintf(stderr, "model '%s' not in mdlmap\n", val);
          continue;
        }
        entclass->mdlnums[entclass->num_mdlnums++] = id;
      } else if (!strcmp(key, "sfx")) {
        assert(entclass->num_sfxnums < MAX_ENT_SFX);
        id = qsfxmap_id_for_name(val);
        if (id <= 0) {
          fprintf(stderr, "sfx '%s' not in sfxmap\n", val);
          continue;
        }
        entclass->sfxnums[entclass->num_sfxnums++] = id;
      }
    }
  }

  fclose(f);
  return ret;
}

const char *qent_parse(qent_t *ent, const char *data) {
  char key[MAX_TOKEN] = { 0 };
  char value[MAX_TOKEN] = { 0 };

  ent->numfields = 0;

  while (1) {
    // parse key
    data = com_parse(data, key);
    if (key[0] == '}')
      break;

    assert(data);

    // parse value
    data = com_parse(data, value);
    assert(value[0] != '}');
    assert(data);

    // save keyvalue
    assert(ent->numfields < MAX_ENT_FIELDS);
    const int keylen = strlen(key);
    const int valuelen = strlen(value);
    qentfield_t *field = calloc(1, sizeof(qentfield_t) + keylen + valuelen + 2);
    assert(field);
    field->keylen = keylen;
    field->valuelen = valuelen;
    field->key = field->strdata;
    field->value = field->strdata + keylen + 1;
    memcpy(field->key, key, keylen);
    memcpy(field->value, value, valuelen);
    ent->fields[ent->numfields++] = field;

    // save pointer to classname if that's what it is
    if (!strcmp(key, "classname"))
      ent->classname = field->value;
  }

  assert(ent->classname);

  ent->info = qentmap_find(ent->classname);

  assert(ent->info);

  return data;
}

void qent_load(const char *data, int datalen) {
  char token[MAX_TOKEN] = { 0 };
  const char *end = data + datalen;
  qent_t *ent = NULL;

  while (1) {
    // parse opening bracket
    data = com_parse(data, token);
    if (!data || data >= end)
      break;
    assert(token[0] == '{');

    // parse entity block
    assert(num_qents < MAX_ENTITIES);
    ent = &qents[num_qents++];
    data = qent_parse(ent, data);
  }
}

const char *qent_get_string(qent_t *ent, const char *key) {
  for (int i = 0; i < ent->numfields; ++i) {
    if (!strcmp(ent->fields[i]->key, key))
      return ent->fields[i]->value;
  }
  return NULL;
}

const char *qent_get_int(qent_t *ent, const char *key, int *out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  *out = atoi(value);
  return value;
}

const char *qent_get_float(qent_t *ent, const char *key, float *out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  *out = atof(value);
  return value;
}

const char *qent_get_vector(qent_t *ent, const char *key, qvec3_t out) {
  const char *value = qent_get_string(ent, key);
  if (!value) return NULL;
  sscanf(value, "%f %f %f", &out[0], &out[1], &out[2]);
  return value;
}
