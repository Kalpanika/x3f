/* X3F_PRINTF.C
 *
 * Library for printout, depending on seriousity level.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_printf.h"

#include <stdio.h>
#include <stdarg.h>

x3f_verbosity_t x3f_printf_level = INFO;

extern void x3f_printf(x3f_verbosity_t level, const char *fmt, ...)
{
  va_list ap;
  FILE *f = level > WARN ? stdout : stderr;

  if (level > x3f_printf_level) return;

  switch(level) {
  case ERR:
    fprintf(f, "ERR: ");
    break;
  case WARN:
    fprintf(f, "WRN: ");
    break;
  case INFO:
    fprintf(f, "   : ");
    break;
  case DEBUG:
    fprintf(f, "dbg: ");
    break;
  }
  va_start(ap, fmt);
  vfprintf(f, fmt, ap);
  va_end(ap);
}
