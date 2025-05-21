#pragma once

#include <stdio.h>
#include "common.h"

#define MAX_ERROR 1024

#define Sys_Printf(...) printf(__VA_ARGS__)
#define ASSERT(x) if (!(x)) Sys_Error("ASSERTION FAILED:\n`%s` at %s:%d", #x, __FILE__, __LINE__)

extern x32 sys_time;
extern x16 sys_delta_time;
extern u32 sys_last_counter;

// initialize low level utilities
void Sys_Init(void);

// updates sys_time etc
void Sys_UpdateTime(void);

// r/o file operations
int Sys_FileOpenRead(const char *path, int *hndl);
void Sys_FileClose(int handle);
void Sys_FileSeek(int handle, int position);
int Sys_FileRead(int handle, void *dest, int count);
qboolean Sys_FileExists(const char *fname);

// an error will cause the entire program to exit
void Sys_Error(const char *error, ...) __attribute__((noreturn));

// installs CPU exception handler (see exception.c)
void Sys_InstallExceptionHandler(void);

// returns vblanks since app start
s32 Sys_Frames(void);

// waits for n vblanks
void Sys_Wait(int n);

// returns seconds since app start in 1.19.12 fixed point
static inline x32 Sys_FixedTime(void) {
  return sys_time;
}

// returns seconds since last Sys_UpdateTime call in 1.19.12 fixed point
static inline x16 Sys_FixedDeltaTime(void) {
  return sys_delta_time;
}
