#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxcd.h>

#include "common.h"
#include "dbgfont.h"
#include "system.h"

static char sys_errormsg[MAX_ERROR];

#define TICKS_PER_SECOND 60 // NTSC on NTSC console

void Sys_Wait(int n) {
  while (n--) VSync(0);
}

// initialize low level utilities
void Sys_Init(void) {
  // initialize interrupts n shit
  ResetGraph(3);
  Sys_InstallExceptionHandler();
}

void Sys_Error(const char *error, ...) {
  // disable ticker
  VSyncCallback(NULL);

  // print time into the buffer first
  const x32 xtime = Sys_FixedTime();
  const int timelen = snprintf(sys_errormsg, sizeof(sys_errormsg), "ERROR (T%d.%04d)\n", xtime >> FIXSHIFT, xtime & (FIXSCALE-1));

  // append the error
  va_list args;
  va_start(args, error);
  vsnprintf(sys_errormsg + timelen, sizeof(sys_errormsg) - timelen, error, args);
  va_end(args);

  // spew to TTY
  printf("%s\n", sys_errormsg);

  // setup graphics viewport and clear screen
  SetDispMask(0);
  DISPENV disp;
  DRAWENV draw;
  SetDefDispEnv(&disp, 0, 0, VID_WIDTH, VID_HEIGHT);
  SetDefDrawEnv(&draw, 0, 0, VID_WIDTH, VID_HEIGHT);
  setRGB0(&draw, 0x40, 0x00, 0x00); draw.isbg = 1;
  PutDispEnv(&disp);
  PutDrawEnv(&draw);

  // init debug printer
  Dbg_PrintReset(8, 8, 0x40, 0x00, 0x00);

  // draw
  Dbg_PrintString(sys_errormsg);
  DrawSync(0);
  VSync(0);
  SetDispMask(1);

  while (1) VSync(0);
}

x32 Sys_FixedTime(void) {
  // number of vblanks -> seconds
  return (VSync(-1) << FIXSHIFT) / TICKS_PER_SECOND;
}

s32 Sys_Frames(void) {
  return VSync(-1);
}

// TEMPORARY CD FILE READING API WITH BUFFERS AND SHIT
// copied straight from d2d-psx and converted to only use one static handle

#define SECSIZE 2048
#define BUFSECS 4
#define BUFSIZE (BUFSECS * SECSIZE)
#define MAX_FHANDLES 1

typedef struct cd_file_s {
  CdlFILE cdf;
  s32 secstart, secend, seccur;
  s32 fp, bufp;
  s32 bufleft;
  u8 buf[BUFSIZE];
} cd_file_t;

// lmao 1handle
static cd_file_t fhandle;
static s32 num_fhandles = 0;

int Sys_FileOpenRead(const char *fname, int *outhandle) {
  cd_file_t *f = num_fhandles ? NULL : &fhandle;

  if (f == NULL) {
    Sys_Error("Sys_FileOpenRead(%s): too many file handles\n", fname);
    return -1;
  }

  memset(f, 0, sizeof(*f));

  if (CdSearchFile(&f->cdf, fname) == NULL) {
    Sys_Printf("Sys_FileOpenRead(%s): file not found\n", fname);
    return -2;
  }

  // read first sector of the file
  CdControl(CdlSetloc, (u8 *)&f->cdf.pos, 0);
  CdRead(BUFSECS, (u32 *)f->buf, CdlModeSpeed);
  CdReadSync(0, NULL);

  // set fp and shit
  f->secstart = CdPosToInt(&f->cdf.pos);
  f->seccur = f->secstart;
  f->secend = f->secstart + (f->cdf.size + SECSIZE-1) / SECSIZE;
  f->fp = 0;
  f->bufp = 0;
  f->bufleft = (f->cdf.size >= BUFSIZE) ? BUFSIZE : f->cdf.size;

  num_fhandles++;
  Sys_Printf("Sys_FileOpenRead(%s): size %u\n", fname, f->cdf.size);

  *outhandle = 1;

  return f->cdf.size;
}

qboolean Sys_FileExists(const char *fname) {
  CdlFILE cdf;
  if (CdSearchFile(&cdf, (char *)fname) == NULL) {
    Sys_Printf("Sys_FileExists(%s): file not found\n", fname);
    return false;
  }
  return true;
}

void Sys_FileClose(int h) {
  if (h <= 0) return;
  num_fhandles--;
}

int Sys_FileRead(int handle, void *dest, int size) {
  if (handle <= 0 || !dest) return -1;
  if (!size) return 0;

  cd_file_t *f = &fhandle;
  u8 *ptr = dest;
  s32 rx = 0;
  s32 fleft, rdbuf;
  CdlLOC pos;

  while (size) {
    // first empty the buffer
    rdbuf = (size > f->bufleft) ? f->bufleft : size;
    memcpy(ptr, f->buf + f->bufp, rdbuf);
    rx += rdbuf;
    ptr += rdbuf;
    f->fp += rdbuf;
    f->bufp += rdbuf;
    f->bufleft -= rdbuf;
    size -= rdbuf;

    // if we went over, load next sector
    if (f->bufleft == 0) {
      f->seccur += BUFSECS;
      // check if we have reached the end
      if (f->seccur >= f->secend)
        return rx;
      // looks like you need to seek every time when you use CdRead
      CdIntToPos(f->seccur, &pos);
      CdControl(CdlSetloc, (u8 *)&pos, 0);
      CdRead(BUFSECS, (u32 *)f->buf, CdlModeSpeed);
      CdReadSync(0, 0);
      fleft = f->cdf.size - f->fp;
      f->bufleft = (fleft >= BUFSIZE) ? BUFSIZE: fleft;
      f->bufp = 0;
    }
  }

  return rx;
}

void Sys_FileSeek(int handle, int ofs) {
  if (handle <= 0) return;

  cd_file_t *f = &fhandle;

  if (f->fp == ofs) return;

  s32 fsec = f->secstart + (ofs / BUFSIZE) * BUFSECS;
  s32 bofs = ofs % BUFSIZE;

  // fuck SEEK_END, it's only used to get file length here

  if (fsec != f->seccur) {
    // sector changed; seek to new one and buffer it
    CdlLOC pos;
    CdIntToPos(fsec, &pos);
    CdControl(CdlSetloc, (u8 *)&pos, 0);
    CdRead(BUFSECS, (u32 *)f->buf, CdlModeSpeed);
    CdReadSync(0, 0);
    f->seccur = fsec;
    f->bufp = -1; // hack: see below
  }

  if (bofs != f->bufp) {
    // buffer offset changed (or new sector loaded); reset pointers
    f->bufp = bofs;
    f->bufleft = BUFSIZE - bofs;
    if (f->bufleft < 0) f->bufleft = 0;
  }

  f->fp = ofs;
}
