#pragma once

#include <stdio.h>
#include "common.h"

#define MAX_ERROR 1024
extern char sys_errormsg[MAX_ERROR];

#define Sys_Printf(...) printf(__VA_ARGS__)
#define Sys_Error(...) do { sprintf(sys_errormsg, __VA_ARGS__); Sys_Panic(sys_errormsg); } while (0)
#define ASSERT(x) if (!(x)) Sys_Error("ASSERTION FAILED:\n`%s` at %s:%d", #x, __FILE__, __LINE__)

// initialize low level utilities
void Sys_Init(void);

// r/o file operations
int Sys_FileOpenRead(const char *path, int *hndl);
void Sys_FileClose(int handle);
void Sys_FileSeek(int handle, int position);
int Sys_FileRead(int handle, void *dest, int count);
qboolean Sys_FileExists(const char *fname);

// an error will cause the entire program to exit
void Sys_Panic(const char *error) __attribute__((noreturn));

// installs CPU exception handler (see exception.c)
void Sys_InstallExceptionHandler(void);

// returns seconds since start in 1.19.12 fixed point
x32 Sys_FixedTime(void);

void Sys_Wait(int n);
