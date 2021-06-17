#pragma once

#include "common.h"
#include "model.h"

#define MAX_ENT_LEAFS 16

typedef struct edict_s edict_t;

typedef void (*think_fn_t)(edict_t *self);
typedef void (*interact_fn_t)(edict_t *self, edict_t *other);

typedef struct entvars_s {
  u8 classname;
  model_t *model;
  u8 extra[];
} entvars_t;

struct edict_s {
  qboolean free;
  link_t area;
  s8 numleafs;
  s16 leafnums[MAX_ENT_LEAFS];
  x32 freetime;
  entvars_t v;
};

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, edict_t, area)

edict_t *ed_alloc(void);
void ed_free(edict_t *ed);
