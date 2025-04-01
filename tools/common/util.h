#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "psxtypes.h"

#define PSXRGB(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5) | ((r) >> 3))
#define PSXTPAGE(tp, abr, x, y) ((((x)&0x3FF)>>6) | (((y)>>8)<<4) | (((abr)&0x3)<<5) | (((tp)&0x3)<<7))

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define MAX_VA_LEN 4096
#define MAX_VA_BUF 4

#ifdef _WIN32
#define qmkdir(dirn, dirp) mkdir(dirn)
#else
#define qmkdir(dirn, dirp) mkdir(dirn, dirp)
#endif

const char *strfmt(const char *fmt, ...);
void panic(const char *fmt, ...);

void *lmp_read(const char *dir, const char *fname, size_t *outsize);

void convert_palette(u16 *dstpal, const u8 *pal, const int numcolors);
