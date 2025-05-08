#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "fixed.h"
#include "vector.h"
#include "mathlib.h"
#include "memory.h"

// extra types
typedef struct link_s {
  struct link_s *prev, *next;
} link_t;

#define STRUCT_FROM_LINK(l, t, m) ((t *)((u8 *)l - (int)&(((t *)0)->m)))

// general limits
#define MAX_EDICTS      600
#define MAX_SOUNDS      255 // +1 invalid
#define MAX_MODELS      255 // +1 invalid
#define MAX_TEX_WIDTH   256
#define MAX_TEX_HEIGHT  256
#define MAX_OSPATH      64
#define MAX_PLAYERS     1
#define MAX_LIGHTSTYLES 64
#define MAX_USERSTYLES  16
#define MAX_STYLESTRING 64
#define MAX_VA_STRING   1024

// buttons
#define BTN_FIRE 1
#define BTN_JUMP 2

// graphics-related
#define VID_WIDTH      320
#define VID_HEIGHT     240
#define VID_CENTER_X   (VID_WIDTH >> 1)
#define VID_CENTER_Y   (VID_HEIGHT >> 1)
#define VID_NUM_COLORS 256
#define VID_MAXFPS     60 // TODO: PAL?

// texture flags
#define TEX_SPECIAL   1
#define TEX_LIQUID    2
#define TEX_SKY       4
#define TEX_INVISIBLE 8
#define TEX_ANIMATED  16
#define TEX_NULL      0x8000

// VRAM layout
#define VRAM_PIC_XSTART 960
#define VRAM_PIC_WIDTH  64
#define VRAM_PIC_HEIGHT 512
#define VRAM_TEX_XSTART 320
#define VRAM_TEX_YSTART 0
#define VRAM_TEX_WIDTH  (1024 - VRAM_PIC_WIDTH - VRAM_TEX_XSTART)
#define VRAM_TEX_HEIGHT 512
#define VRAM_PAL_XSTART 0
#define VRAM_PAL_YSTART VID_HEIGHT
#define VRAM_PAL_WIDTH  VID_NUM_COLORS
#define VRAM_PAL_HEIGHT 1

// filesystem stuff
#define FS_BASE "\\ID1PSX"

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

FORCEINLINE void ClearLink(link_t *l) {
  l->prev = l->next = l;
}

FORCEINLINE void RemoveLink(link_t *l) {
  l->next->prev = l->prev;
  l->prev->next = l->next;
}

FORCEINLINE void InsertLinkBefore(link_t *l, link_t *before) {
  l->next = before;
  l->prev = before->prev;
  l->prev->next = l;
  l->next->prev = l;
}

FORCEINLINE void InsertLinkAfter(link_t *l, link_t *after) {
  l->next = after->next;
  l->prev = after;
  l->prev->next = l;
  l->next->prev = l;
}

char *COM_SkipPath(char *pathname);
void COM_StripExtension(char *in, char *out);
void COM_FileBase(char *in, char *out);
void COM_DefaultExtension(char *path, char *extension);

extern char com_gamedir[MAX_OSPATH];
extern char com_vastring[MAX_VA_STRING];

#define VA(...) ({ sprintf(com_vastring, __VA_ARGS__); com_vastring; })

void COM_Init(void);

extern qboolean com_registered;
