#pragma once

#include "../common/psxtypes.h"
#include "../common/idbsp.h"
#include "../common/psxbsp.h"

#define MAX_ALIASES 3
#define MAX_ENT_MDLS 32
#define MAX_ENT_SFX 32
#define MAX_ENT_CLASSES 0x80
#define MAX_ENT_CLASSNAME 64
#define MAX_ENT_FIELDS 64

enum qfieldtype_e {
  QFT_STRING,
  QFT_VECTOR,
  QFT_FLOAT,
};

typedef struct {
  char classname[MAX_ENT_CLASSNAME];

  int classnum;

  int num_mdlnums;
  int mdlnums[MAX_ENT_MDLS][MAX_ALIASES];

  int num_sfxnums;
  int sfxnums[MAX_ENT_SFX][MAX_ALIASES];
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

extern int num_qents;
extern qent_t qents[MAX_ENTITIES];

int qentmap_init(const char *mapfile);
qentmap_t *qentmap_find(const char *classname);
int qentmap_link(const char *resfile);

void qent_load(const char *data, int datalen);

const char *qent_get_string(qent_t *ent, const char *key);
const char *qent_get_int(qent_t *ent, const char *key, int *out);
const char *qent_get_float(qent_t *ent, const char *key, float *out);
const char *qent_get_vector(qent_t *ent, const char *key, qvec3_t out);
