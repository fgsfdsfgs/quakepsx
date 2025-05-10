#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>

#include "common.h"
#include "dbgfont.h"
#include "exception.h"

#define EX_PRINTF(...) \
  snprintf(com_vastring, sizeof(com_vastring), __VA_ARGS__); \
  Dbg_PrintString(com_vastring);

static u32 exception_event;

static inline const char *ExceptionCause(const u32 cr) {
  static const char *msg[] = {
    "EXT INT",
    "?",
    "?",
    "?",
    "READ ERR",
    "WRITE ERR",
    "CMD BUS ERR",
    "DATA BUS ERR",
    "SYSCALL",
    "BREAK",
    "UNK COMMAND",
    "NO COP",
    "OVERFLOW",
  };
  return msg[(cr & 63) >> 2];
}

static u32 GetCause(void) {
  u32 cr = 0;
  __asm__ volatile("mfc0 %0, $13": "=r"(cr) ::);
  return cr;
}

static void ExceptionFunc(void) {
  // don't double except
  static volatile int inexception = false;
  if (inexception) {
    printf("DOUBLE FAULT\n");
    while (1);
  }
  inexception = true;

  // get current thread's TCB and make a copy of the relevant data
  const struct ToT *tot = (const struct ToT *)0x100;
  const struct TCB *tcb_list = (const struct TCB *)tot[2].head;
  const u32 status = tcb_list[0].status;
  const u32 cr = GetCause();
  u32 regs[40];
  memcpy(regs, tcb_list[0].reg, sizeof(regs));

  // spew to tty
  printf("UH OH ZONE\nSTATUS=%08x\nCR=%08x\nPC=%08x\nRA=%08x\n", status, cr, regs[R_EPC], regs[R_RA]);

  // setup graphics viewport and clear screen
  DISPENV disp;
  DRAWENV draw;
  ResetGraph(0);
  SetDispMask(0);
  SetDefDispEnv(&disp, 0, 0, VID_WIDTH, VID_HEIGHT);
  SetDefDrawEnv(&draw, 0, 0, VID_WIDTH, VID_HEIGHT);
  setRGB0(&draw, 0x40, 0x00, 0x00); draw.isbg = 1; draw.dtd = 1;
  PutDispEnv(&disp);
  PutDrawEnv(&draw);

  // init debug printer
  Dbg_PrintReset(8, 16, 0x40, 0x00, 0x00);

  // the most important info
  const char *cause = ExceptionCause(cr);
  EX_PRINTF("UH OH ZONE\n\n");
  EX_PRINTF("CAUSE:  %s / %08x\nSTATUS: %08x\n\n", cause, cr, status);
  EX_PRINTF("PC=%08x  RA=%08x\n\n", regs[R_EPC],regs[R_RA]);

  // set display mask right away so that we can at least see the header
  SetDispMask(1);

  // GPR dump
  EX_PRINTF("AT=%08x  K0=%08x  K1=%08x\n", regs[R_AT],  regs[R_K0],  regs[R_K1]);
  EX_PRINTF("A0=%08x  A1=%08x  A2=%08x\n", regs[R_A0],  regs[R_A1],  regs[R_A2]);
  EX_PRINTF("A3=%08x  V0=%08x  V1=%08x\n", regs[R_A3],  regs[R_V0],  regs[R_V1]);
  EX_PRINTF("T0=%08x  T1=%08x  T2=%08x\n", regs[R_T0],  regs[R_T1],  regs[R_T2]);
  EX_PRINTF("T3=%08x  T4=%08x  T5=%08x\n", regs[R_T3],  regs[R_T4],  regs[R_T5]);
  EX_PRINTF("T6=%08x  T7=%08x  T8=%08x\n", regs[R_T6],  regs[R_T7],  regs[R_T8]);
  EX_PRINTF("T9=%08x  S0=%08x  S1=%08x\n", regs[R_T9],  regs[R_S0],  regs[R_S1]);
  EX_PRINTF("S2=%08x  S3=%08x  S4=%08x\n", regs[R_S2],  regs[R_S3],  regs[R_S4]);
  EX_PRINTF("S5=%08x  S6=%08x  S7=%08x\n", regs[R_S5],  regs[R_S6],  regs[R_S7]);
  EX_PRINTF("GP=%08x  FP=%08x  SP=%08x\n", regs[R_GP],  regs[R_FP],  regs[R_SP]);

  // dump stack if possible
  u32 sp = regs[R_SP] & ~0x0Fu;
  if (sp > 0x80100000u && sp < 0x80200000u) {
    EX_PRINTF("\nSTACK FROM %08x\n\n", sp);
    for (u32 i = 0; sp < 0x80200000u && i < 15; sp += 16, ++i) {
      const u32 *data = (const u32 *)sp;
      EX_PRINTF("%02x %08x %08x %08x %08x\n", sp & 0xFF, data[0], data[1], data[2], data[3]);
    }
  }

  while (1);
}

void Sys_InstallExceptionHandler(void) {
  // HwCPU fires when there's an unhandled CPU exception
  EnterCriticalSection();
  exception_event = OpenEvent(HwCPU, EvSpTRAP, EvMdINTR, ExceptionFunc);
  EnableEvent(exception_event);
  ExitCriticalSection();

  printf("Sys_InstallExceptionHandler(): opened HwCPU event %08x\n", exception_event);

  /*
  // alternatively:
  // A(40h) - SystemErrorUnresolvedException()
  void **a0tab = (void **)0x200;
  const void *old = a0tab[0x40];
  a0tab[0x40] = exception_cb;
  printf("ex_install_handler(): hooked exception handler: %p -> %p\n", old, exception_cb);
  */
}
