#include <sys/types.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

#include "common.h"
#include "game.h"
#include "move.h"
#include "render.h"

#define P_FRAMETIME1 PR_FRAMETIME
#define P_FRAMETIME2 (PR_FRAMETIME / 2)
#define P_GRAVITY XMUL16(G_GRAVITY, FTOX(0.05))

static const u8 ramp1[] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
static const u8 ramp2[] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
static const u8 ramp3[] = { 0x6d, 0x6b, 6, 5, 4, 3 };

void R_SpawnParticleEffect(const u8 type, const x32vec3_t *org, const x32vec3_t *vel, const u8 color, s16 count) {
  s16 scale;
  if (count > 100)
    scale = 3;
  else if (count > 20)
    scale = 2;
  else
    scale = 1;

  const s16vec3_t sorg = {{ org->x >> FIXSHIFT, org->y >> FIXSHIFT, org->z >> FIXSHIFT }};
  const s16vec3_t svel = {{ XMUL16(15, vel->x), XMUL16(15, vel->y), XMUL16(15, vel->z) }};

  particle_t *p = rs.particles;
  int i;
  for (i = 0; i < MAX_PARTICLES && count; ++i, ++p) {
    if (p->die > 0)
      continue;

    p->type = type;
    p->color = (color & ~7) + (rand() & 7);
    p->ramp = 0;
    p->die = 1 + (rand() & 3);

    p->org.x = sorg.x + scale * ((rand() & 15) - 8);
    p->org.y = sorg.y + scale * ((rand() & 15) - 8);
    p->org.z = sorg.z + scale * ((rand() & 15) - 8);

    p->vel = svel;

    --count;
  }

  if (i > rs.num_particles)
    rs.num_particles = i;
}

void R_SpawnParticleExplosion(const x32vec3_t *org) {
  const s16vec3_t sorg = {{ org->x >> FIXSHIFT, org->y >> FIXSHIFT, org->z >> FIXSHIFT }};

  s16 count = 128;
  particle_t *p = rs.particles;
  int i;
  for (i = 0; i < MAX_PARTICLES && count; ++i, ++p) {
    if (p->die > 0)
      continue;

    p->type = PT_EXPLODE + (i & 1);
    p->color = ramp1[0];
    p->ramp = 0;
    p->die = 4;

    p->org.x = sorg.x + ((rand() & 31) - 16);
    p->org.y = sorg.y + ((rand() & 31) - 16);
    p->org.z = sorg.z + ((rand() & 31) - 16);

    p->vel.x = (rand() & 511) - 256;
    p->vel.y = (rand() & 511) - 256;
    p->vel.z = (rand() & 511) - 256;

    --count;
  }

  if (i > rs.num_particles)
    rs.num_particles = i;
}

void R_SpawnParticleLavaSplash(const x32vec3_t *org) {
  s16 i, j, n;
  particle_t *p = rs.particles;
  const s16vec3_t sorg = {{ org->x >> FIXSHIFT, org->y >> FIXSHIFT, org->z >> FIXSHIFT }};

  n = 0;
  for (i = -16; i < 16; i += 2) {
    for (j = -16; j < 16; j += 2) {
      for (; n < MAX_PARTICLES; ++n, ++p) {
        if (p->die > 0)
          continue;
        p->ramp = 0;
        p->die = 40 + (rand() & 3);
        p->color = 224 + (rand() & 7);
        p->type = PT_GRAV;
        p->vel.x = i * 8 + (rand() & 7);
        p->vel.y = j * 8 + (rand() & 7);
        p->vel.z = 32 + (rand() & 31);
        p->org.x = sorg.x + p->vel.x;
        p->org.y = sorg.y + p->vel.y;
        p->org.z = sorg.z + (rand() & 63);
        break;
      }
      if (n == MAX_PARTICLES)
        goto _end;
    }
  }

_end:
  if (n > rs.num_particles)
    rs.num_particles = n;
}

void R_SpawnParticleTeleport(const x32vec3_t *org) {
  s16 i, j, k, n;
  particle_t *p = rs.particles;

  const s16vec3_t sorg = {{ org->x >> FIXSHIFT, org->y >> FIXSHIFT, org->z >> FIXSHIFT }};

  n = 0;
  for (i = -16; i < 16; i += 8) {
    for (j = -16; j < 16; j += 8) {
      for (k = -16; k < 16; k += 8) {
        for (; n < MAX_PARTICLES; ++n, ++p) {
          if (p->die > 0)
            continue;
          p->ramp = 0;
          p->die = 1 + (rand() & 7);
          p->color = 7 + (rand() & 7);
          p->type = PT_GRAV;
          p->org.x = sorg.x + i + (rand() & 3);
          p->org.y = sorg.y + j + (rand() & 3);
          p->org.z = sorg.z + k + (rand() & 3);
          p->vel.x = i * (8 + (rand() & 63));
          p->vel.y = j * (8 + (rand() & 63));
          p->vel.z = k * (8 + (rand() & 63));
          break;
        }
        if (n == MAX_PARTICLES)
          goto _end;
      }
    }
  }

_end:
  if (n > rs.num_particles)
    rs.num_particles = n;
}

void R_SpawnParticleTrail(const x32vec3_t *org, const x32vec3_t *oldorg, const u8 type) {
  x32vec3_t delta;
  XVecSub(org, oldorg, &delta);

  if (!delta.x && !delta.y && !delta.z)
    return;

  particle_t *p = rs.particles;
  s16vec3_t sorg;
  x16 d = 0;
  for (int n = 0; n < 2; ++n, d += (ONE / 2)) {
    sorg.x = (oldorg->x + xmul32(d, delta.x)) >> FIXSHIFT;
    sorg.y = (oldorg->y + xmul32(d, delta.y)) >> FIXSHIFT;
    sorg.z = (oldorg->z + xmul32(d, delta.z)) >> FIXSHIFT;

    s16 count = 1;
    for (; count && p < rs.particles + MAX_PARTICLES; ++p) {
      if (p->die > 0)
        continue;

      --count;

      p->vel.x = 0;
      p->vel.y = 0;
      p->vel.z = 0;

      p->org.x = sorg.x + ((rand() & 7) - 4);
      p->org.y = sorg.y + ((rand() & 7) - 4);
      p->org.z = sorg.z + ((rand() & 7) - 4);

      p->die = 8;

      switch (type) {
      case EF_ROCKET: // rocket trail
        p->ramp = rand() & 3;
        p->color = ramp3[p->ramp];
        p->type = PT_FIRE;
        break;
      case EF_GIB: // blood trail
        p->ramp = 0;
        p->color = 67 + (rand() & 3);
        p->type = PT_GRAV;
        break;
      case EF_TRACER: // green trail
        p->ramp = 0;
        p->color = 52 + (rand() & 7);
        p->type = PT_STATIC;
        break;
      default:
        break;
      }
    }
  }

  const s32 i = p - rs.particles;
  if (i > rs.num_particles)
    rs.num_particles = i;
}

void R_UpdateParticles(void) {
  static x32 accum1;
  static x32 accum2;

  s32 time1 = 0;
  accum1 += gs.frametime;
  if (accum1 >= P_FRAMETIME1) {
    time1 = accum1 / P_FRAMETIME1;
    accum1 = 0;
  }

  s32 time2 = 0;
  accum2 += gs.frametime;
  if (accum2 >= P_FRAMETIME2) {
    time2 = accum2 / P_FRAMETIME2;
    accum2 = 0;
  }

  const x16 frametime = gs.frametime;
  const s16 grav = XMUL16(P_GRAVITY >> FIXSHIFT, frametime);
  const s16 dvel = XMUL16(4, frametime);

  particle_t *p = rs.particles;
  s16 last_active = -1;
  for (int i = 0; i < rs.num_particles; ++i, ++p) {
    if (p->die <= 0)
      continue;

    if (i > last_active)
      last_active = i;

    p->org.x += XMUL16(frametime, p->vel.x);
    p->org.y += XMUL16(frametime, p->vel.y);
    p->org.z += XMUL16(frametime, p->vel.z);

    switch (p->type) {
    case PT_FIRE:
      p->ramp += time1;
      if (p->ramp >= 6)
        p->die = -1;
      else
        p->color = ramp3[p->ramp];
      p->vel.z += grav;
      break;

    case PT_EXPLODE:
      p->ramp += time2;
      if (p->ramp >= 8)
        p->die = -1;
      else
        p->color = ramp1[p->ramp];
      p->vel.x += dvel * p->vel.x;
      p->vel.y += dvel * p->vel.y;
      p->vel.z += dvel * p->vel.z - grav;
      break;

    case PT_EXPLODE2:
      p->ramp += time2;
      if (p->ramp >= 8)
        p->die = -1;
      else
        p->color = ramp2[p->ramp];
      p->vel.x -= XMUL16(p->vel.x, frametime);
      p->vel.y -= XMUL16(p->vel.y, frametime);
      p->vel.z -= XMUL16(p->vel.z, frametime) + grav;
      break;

    case PT_GRAV:
      p->die -= time2;
      p->vel.z -= grav;
      break;

    case PT_STATIC:
      p->die -= time2;
      break;

    default:
      break;
    }
  }

  rs.num_particles = last_active + 1;

  beam_t *b = rs.beams;
  for (int i = 0; i < MAX_BEAMS; ++i, ++b) {
    if (b->die > 0)
      b->die -= time1;
  }
}

void R_SpawnBeam(const x32vec3_t *src, const x32vec3_t *dst, const u32 color, const s16 frames, s16 index) {
  if (index < 0) {
    if (++rs.last_beam == MAX_BEAMS)
      rs.last_beam = 1;
    index = rs.last_beam;
  }
  beam_t *b = rs.beams + index;
  b->die = frames;
  b->color = color;
  b->src.x = src->x >> FIXSHIFT;
  b->src.y = src->y >> FIXSHIFT;
  b->src.z = src->z >> FIXSHIFT;
  b->dst.x = dst->x >> FIXSHIFT;
  b->dst.y = dst->y >> FIXSHIFT;
  b->dst.z = dst->z >> FIXSHIFT;
}
