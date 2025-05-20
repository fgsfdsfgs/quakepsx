#include <psxspu.h>

#include "types.h"
#include "spu.h"

// still faster to operate the voices using the registers
// than to get through libspu's attribute bullshit

#define SPU_VOICE_BASE ((volatile u16 *)(0x1F801C00))
#define SPU_KEY_ON_LO ((volatile u16 *)(0x1F801D88))
#define SPU_KEY_ON_HI ((volatile u16 *)(0x1F801D8A))
#define SPU_KEY_OFF_LO ((volatile u16 *)(0x1F801D8C))
#define SPU_KEY_OFF_HI ((volatile u16 *)(0x1F801D8E))
#define SPU_KEY_END ((volatile u32 *)(0x1F801D9C))

#define SPU_KEYCH(x) (0x1L<<(x))
#define SPU_VOICECH(x) SPU_KEYCH(x)

struct SPU_voice {
  volatile s16 vol_left;
  volatile s16 vol_right;
  volatile u16 sample_rate;
  volatile u16 sample_startaddr;
  volatile u16 attack_decay;
  volatile u16 sustain_release;
  volatile u16 vol_current;
  volatile u16 sample_repeataddr;
};
#define SPU_VOICE(x) (((volatile struct SPU_voice *)SPU_VOICE_BASE) + (x))

#define PAN_SHIFT 8

u32 s_spuram_ptr = SPU_RAM_BASE;

void SPU_Init(void) {
  SpuInit();
  SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
  SpuSetCommonMasterVolume(0x3FFF, 0x3FFF);
  SPU_ClearAllVoices();
  s_spuram_ptr = SPU_RAM_BASE;
}

void SPU_KeyOn(const u32 mask) {
  *SPU_KEY_ON_LO = mask;
  *SPU_KEY_ON_HI = mask >> 16;
}

void SPU_KeyOff(const u32 mask) {
  *SPU_KEY_OFF_LO = mask;
  *SPU_KEY_OFF_HI = mask >> 16;
}

void SPU_ClearVoice(const u32 v) {
  SPU_VOICE(v)->vol_left = 0;
  SPU_VOICE(v)->vol_right = 0;
  SPU_VOICE(v)->sample_rate = 0;
  SPU_VOICE(v)->sample_startaddr = 0;
  SPU_VOICE(v)->sample_repeataddr = 0;
  SPU_VOICE(v)->attack_decay = 0x000F;
  SPU_VOICE(v)->sustain_release = 0x0000;
  SPU_VOICE(v)->vol_current = 0;
}

void SPU_ClearAllVoices(void) {
  SPU_KeyOff(0xFFFFFFFFu);
  for (u32 i = 0; i < SPU_NUM_VOICES; ++i)
    SPU_ClearVoice(i);
}

void SPU_SetVoiceVolume(const u32 v, const s16 lvol, const s16 rvol) {
  SPU_VOICE(v)->vol_left = lvol;
  SPU_VOICE(v)->vol_right = rvol;
}

u32 SPU_GetVoiceEndMask(void) {
  return *SPU_KEY_END;
}

void SPU_PlaySample(const u32 ch, const u32 addr, const u32 freq) {
  SPU_VOICE(ch)->sample_rate = FreqToPitch(freq);
  SPU_VOICE(ch)->sample_startaddr = (addr >> 3);
  SPU_KeyOn(SPU_VOICECH(ch)); // this restarts the channel on the new address
}

void SPU_WaitForTransfer(void) {
  SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
}

void SPU_StartUpload(const u32 dstaddr, const u8 *src, const u32 size) {
  SpuSetTransferStartAddr(dstaddr);
  SpuWrite((u32 *)src, size);
}

void SPU_EnableCDDA(const qboolean enable) {
  if (enable)
    SPU_CTRL |= 1;
  else
    SPU_CTRL &= ~1;
}
