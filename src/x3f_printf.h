/* X3F_PRINTF.H
 *
 * Library for printout, depending on seriousity level.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_PRINTF_H
#define X3F_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {ERR=0, WARN=1, INFO=2, DEBUG=3} x3f_verbosity_t;

extern x3f_verbosity_t x3f_printf_level;

extern void x3f_printf(x3f_verbosity_t level, const char *fmt, ...)
  __attribute__((format(printf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif
