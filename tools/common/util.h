#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_VA_LEN 4096
#define MAX_VA_BUF 4

static inline const char *strfmt(const char *fmt, ...) {
  static char buf[MAX_VA_BUF][MAX_VA_LEN];
  static int bufidx = 0;
  bufidx = (bufidx + 1) % MAX_VA_BUF;
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf[bufidx], sizeof(buf[bufidx]), fmt, args);
  va_end(args);
  return buf[bufidx];
}
