#pragma once

#include "../common/psxtypes.h"
#include "../common/psxmdl.h"
#include "qmdl.h"

void xbsp_entmodel_add_from_qmdl(qmdl_t *qm);
void xbsp_entmodel_add_from_qbsp(qbsp_t *qm, s16 id);
xaliashdr_t *xbsp_entmodel_find(const s16 id);
