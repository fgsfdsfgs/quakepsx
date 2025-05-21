#pragma once

#include "common.h"
#include "render.h"

#define MAX_SCR_LINE 128
#define SCR_LINE_TIME TO_FIX32(2)
#define SCR_FLASH_TIME (ONE / 3)

#define FNT_SMALL_W 8
#define FNT_SMALL_H 8
#define FNT_BIG_W 24
#define FNT_BIG_H 24

#define C_WHITE 0x00808080u
#define C_GREY 0x00404040u
#define C_RED 0x00000080u
#define C_DKRED 0x00000030u
#define C_YELLOW 0x00008080u
#define C_DKYELLOW 0x00003030u

void Scr_Init(void);

void Scr_DrawScreen(const int debug_mode);

void Scr_DrawText(const s16 x, const s16 y, const u32 rgb, const char *str);
void Scr_DrawTextOffset(const u8 chofs, const s16 x, const s16 y, const u32 rgb, const char *str);
void Scr_DrawDigits(const s16 x, const s16 y, const u32 rgb, const char *str);
void Scr_DrawPic(const s16 x, const s16 y, const u32 rgb, const pic_t *pic);
void Scr_DrawRect(const s16 x, const s16 y, const s16 w, const s16 h, const u32 rgb, const u8 blend);
void Scr_DrawBlendAdd(const u8 r, const u8 g, const u8 b);
void Scr_DrawBlendHalf(const u8 r, const u8 g, const u8 b);

void Scr_SetTopMsg(const char *str);
void Scr_SetCenterMsg(const char *str);
void Scr_SetBlend(const u32 color, const x32 time);

void Scr_BeginLoading(void);
void Scr_TickLoading(void);
void Scr_EndLoading(void);
