#pragma once

#include "types.h"
#include "fixed.h"
#include "vector.h"
#include "model.h"

enum angleidx_e { PITCH, YAW, ROLL };

void AngleVectors(const x16vec3_t *angles, x16vec3_t *forward, x16vec3_t *right, x16vec3_t *up);
int BoxOnPlaneSide(const x32vec3_t *emins, const x32vec3_t *emaxs, const mplane_t *p);
