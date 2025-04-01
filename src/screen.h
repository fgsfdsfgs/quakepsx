#pragma once

#include "common.h"
#include "render.h"

#define MAX_SCR_LINE 64
#define SCR_LINE_TIME TO_FIX32(2)

#define FNT_SMALL_W 8
#define FNT_SMALL_H 8
#define FNT_BIG_W 24
#define FNT_BIG_H 24

#define ICON_W 24
#define ICON_H 24

#define C_WHITE 0x00808080u
#define C_RED 0x00000080u

void Scr_Init(void);

void Scr_DrawScreen(const int debug_mode);

void Scr_DrawText(const s16 x, const s16 y, const u32 rgb, const char *str);
void Scr_DrawDigits(const s16 x, const s16 y, const u32 rgb, const char *str);
void Scr_DrawPic(const s16 x, const s16 y, const u32 rgb, const pic_t *pic);
void Scr_DrawRect(const s16 x, const s16 y, const s16 w, const s16 h, const u32 rgb, const u8 blend);

void Scr_SetTopMsg(const char *str);
void Scr_SetCenterMsg(const char *str);

void Scr_BeginLoading(void);
void Scr_TickLoading(void);
void Scr_EndLoading(void);
