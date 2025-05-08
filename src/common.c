#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "system.h"

qboolean com_registered = false;

char com_vastring[MAX_VA_STRING];

static void COM_InitFilesystem(void) {
  com_registered = Sys_FileExists(FS_BASE "\\MAPS\\E2M1.PSB;1");
}

void COM_Init(void) {
  COM_InitFilesystem();
}
