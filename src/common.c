#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "system.h"

qboolean registered = false;

char com_vastring[MAX_VA_STRING];

static void COM_InitFilesystem(void) {
  registered = Sys_FileExists(FS_BASE "\\MAPS\\E2M1.PSB;1");
}

void COM_Init(void) {
  COM_InitFilesystem();
}

void ClearLink(link_t *l) {
  l->prev = l->next = l;
}

void RemoveLink(link_t *l) {
  l->next->prev = l->prev;
  l->prev->next = l->next;
}

void InsertLinkBefore(link_t *l, link_t *before) {
  l->next = before;
  l->prev = before->prev;
  l->prev->next = l;
  l->next->prev = l;
}

void InsertLinkAfter(link_t *l, link_t *after) {
  l->next = after->next;
  l->prev = after;
  l->prev->next = l;
  l->next->prev = l;
}

