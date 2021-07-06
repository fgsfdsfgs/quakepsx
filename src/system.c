#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <psxcd.h>

#include "common.h"
#include "system.h"

#define TICKS_PER_SECOND (60 << FIXSHIFT) // NTSC on NTSC console, 1.19.12 fixed point
#define SECONDS_PER_TICK XDIV(ONE, TICKS_PER_SECOND) // 1.19.12 fixed point

void Sys_Wait(int n) {
  while (n--) VSync(0);
}

// initialize low level utilities
void Sys_Init(void) {
  // initialize interrupts n shit
  ResetGraph(3);

  // start up the CD drive
  CdInit();
  // look alive
  CdControl(CdlNop, 0, 0);
  CdStatus();
  // set hispeed mode
  u32 cdmode = CdlModeSpeed;
  CdControlB(CdlSetmode, (u8 *)&cdmode, 0);
  Sys_Wait(3); // have to do this to not explode the drive apparently
}

void Sys_Error(const char *error, ...) {
  char buf[1024];

  VSyncCallback(NULL); // disable ticker

  va_list args;
  va_start(args, error);
  vsnprintf(buf, sizeof(buf), error, args);
  va_end(args);

  const x32 xtime = Sys_FixedTime();

  // spew to TTY
  Sys_Printf("ERROR (T%d.%04d): %s\n", xtime >> FIXSHIFT, xtime & (FIXSCALE-1), buf);

  // setup graphics viewport and clear screen
  SetDispMask(0);
  DISPENV disp;
  DRAWENV draw;
  SetDefDispEnv(&disp, 0, 0, VID_WIDTH, VID_HEIGHT);
  SetDefDrawEnv(&draw, 0, 0, VID_WIDTH, VID_HEIGHT);
  setRGB0(&draw, 0x40, 0x00, 0x00); draw.isbg = 1;
  PutDispEnv(&disp);
  PutDrawEnv(&draw);

  // load built in font
  FntLoad(960, 0);
  FntOpen(0, 8, 320, 224, 0, 100);

  // draw
  FntPrint(-1, "ERROR (T%d.%04d):\n%s", xtime >> FIXSHIFT, xtime & (FIXSCALE-1), buf);
  FntFlush(-1);
  DrawSync(0);
  VSync(0);
  SetDispMask(1);

  while (1) VSync(0);
}

x32 Sys_FixedTime(void) {
  // number of vblanks -> seconds
  return XDIV16(FIX(VSync(-1)), TICKS_PER_SECOND);
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
  CdRead(BUFSECS, (u_long *)f->buf, CdlModeSpeed);
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

s32 Sys_FileRead(int handle, void *dest, int size) {
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
      CdRead(BUFSECS, (u_long *)f->buf, CdlModeSpeed);
      CdReadSync(0, 0);
      fleft = f->cdf.size - f->fp;
      f->bufleft = (fleft >= BUFSIZE) ? BUFSIZE: fleft;
      f->bufp = 0;
    }
  }

  return rx;
}

void Sys_FileSeek(int handle, s32 ofs) {
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
    CdRead(BUFSECS, (u_long *)f->buf, CdlModeSpeed);
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
