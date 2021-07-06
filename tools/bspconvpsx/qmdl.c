#include <stdlib.h>
#include "qmdl.h"
#include "util.h"
#include "../common/psxtypes.h"
#include "../common/idmdl.h"

int qmdl_init(qmdl_t *mdl, u8 *start, const size_t size) {
  mdl->start = start;
  mdl->size = size;

  mdl->header = (qaliashdr_t *)start;
  if (mdl->header->version != ALIAS_VERSION)
    panic("malformed Quake MDL: expected version %d, got %d\n", ALIAS_VERSION, mdl->header->version);

  return 0;
}
