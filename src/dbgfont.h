#include "types.h"

#define DBGFONT_WIDTH 8
#define DBGFONT_HEIGHT 5
#define DBGFONT_START '+'
#define DBGFONT_END 'Z'
#define DBGFONT_NUMCHRS (DBGFONT_END - DBGFONT_START + 1)

extern const u8 sys_dbgfont[DBGFONT_NUMCHRS][DBGFONT_HEIGHT];

void Dbg_PrintReset(const int start_x, const int start_y, const u8 bg_r, const u8 bg_g, const u8 bg_b);
void Dbg_PrintString(const char *s);
