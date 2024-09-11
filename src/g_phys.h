#pragma once

#include "common.h"
#include "entity.h"
#include "world.h"

int G_FlyMove(edict_t *ent, x16 time, const trace_t **steptrace);
void G_WalkMove(edict_t *ent);
