#pragma once

#include "types.h"

#define SPU_RAM_BASE 0x1100
#define SPU_RAM_SIZE (0x80000 - SPU_RAM_BASE)
#define SPU_NUM_VOICES 24
#define SPU_MAX_VOLUME 0x3FFF

void SPU_Init(void);
void SPU_KeyOn(const u32 mask);
void SPU_KeyOff(const u32 mask);
void SPU_ClearVoice(const u32 v) ;
void SPU_SetVoiceVolume(const u32 v, const s16 lvol, const s16 rvol);
u32 SPU_GetVoiceEndMask(void);
void SPU_PlaySample(const u32 ch, const u32 addr, const u32 freq);
void SPU_WaitForTransfer(void);
void SPU_ClearAllVoices(void);
void SPU_StartUpload(const u32 dstaddr, const u8 *src, const u32 size);
void SPU_EnableCDDA(const qboolean enable);

static inline u16 FreqToPitch(const u32 hz) {
  return (hz << 12) / 44100;
}
