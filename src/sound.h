#pragma once

#include "common.h"
#include "spu.h"

// we downsample everything to this
#define SND_FREQ 11025

#define SND_MAXVOL 255

#define SND_NUM_CH (SPU_NUM_VOICES)
#define SND_NUM_CH_AMBIENT 11
#define SND_NUM_CH_STATIC (1 + SND_NUM_CH_AMBIENT) // 1 for CD audio
#define SND_NUM_CH_DYNAMIC (SND_NUM_CH - SND_NUM_CH_STATIC)

#define SND_CH_CDMUSIC 0
#define SND_CH_AMBIENT0 1
#define SND_CH_DYNAMIC0 SND_NUM_CH_STATIC

#define SND_CLIPDIST 1024 // integer
#define SND_INV_CLIPDIST XRECIP(SND_CLIPDIST)
#define SND_STATICSCALE XRECIP(64)

#define ATTN_NONE   0
#define ATTN_NORM   TO_FIX32(1)
#define ATTN_IDLE   TO_FIX32(2)
#define ATTN_STATIC TO_FIX32(3)

#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

typedef struct {
  s16 id;
  u16 frames;
  u32 spuaddr;
} sfx_t;

void Snd_Init(void);

void Snd_SetBank(const sfx_t *sfx, const int num_sfx, const u8 *spudata, const u32 spudata_size);
void Snd_ResetBank(void);

void Snd_NewMap(void);

const sfx_t *Snd_FindSound(const s16 id);
void Snd_StartSound(const s16 entnum, const s16 entch, const sfx_t *sfx, const x32vec3_t *origin, s16 vol, x32 attn);
void Snd_StaticSound(const sfx_t *sfx, const x32vec3_t *origin, s16 vol, x32 attn);

void Snd_Update(x32vec3_t *lorigin, x16vec3_t *lright);

static inline void Snd_StartSoundId(const s16 entnum, const s16 entch, const s16 sfxid, const x32vec3_t *origin, s16 vol, x32 attn) {
  Snd_StartSound(entnum, entch, Snd_FindSound(sfxid), origin, vol, attn);
}

static inline void Snd_StaticSoundId(const s16 sfxid, const x32vec3_t *origin, s16 vol, x32 attn) {
  Snd_StaticSound(Snd_FindSound(sfxid), origin, vol, attn);
}
