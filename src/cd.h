#pragma once

#include "types.h"

typedef enum {
  CDMODE_NONE,
  CDMODE_DATA,
  CDMODE_AUDIO
} cdmode_t;

// 0 - 128
extern s32 cd_volume;

void CD_Init(void);
void CD_Update(void);

// 0 if in data mode or not playing anything
u32 CD_GetCurrentTrack(void);

qboolean CD_IsPlaybackPaused(void);

// 0 - 128
void CD_SetAudioVolume(u8 vol);

void CD_PlayAudio(const u32 track);
void CD_Pause(const qboolean pause);
void CD_Stop(void);

// blocking
void CD_SetMode(const cdmode_t mode);
