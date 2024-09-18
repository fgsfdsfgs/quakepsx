#pragma once

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/psxbsp.h"

#define MAX_ENT_MDLS 4
#define MAX_ENT_FIELDS 64

enum qfieldtype_e {
  QFT_STRING,
  QFT_VECTOR,
  QFT_FLOAT,
};

typedef struct {
  int classnum;
  const char *classname;
  const char *mdls[MAX_ENT_MDLS];
} qentmap_t;

typedef struct {
  int keylen;
  int valuelen;
  char *key;
  char *value;
  char strdata[];
} qentfield_t;

typedef struct {
  int numfields;
  qentfield_t *fields[MAX_ENT_FIELDS];
  const char *classname; // points to one of the fields' values
  const qentmap_t *info;
} qent_t;

extern const int num_qentmap;
extern const qentmap_t qentmap[];

extern int num_qents;
extern qent_t qents[MAX_ENTITIES];

const qentmap_t *qentmap_find(const char *classname);

void qent_load(const char *data, int datalen);

const char *qent_get_string(qent_t *ent, const char *key);
const char *qent_get_int(qent_t *ent, const char *key, int *out);
const char *qent_get_float(qent_t *ent, const char *key, float *out);
const char *qent_get_vector(qent_t *ent, const char *key, qvec3_t out);
