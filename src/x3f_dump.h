/* X3F_DUMP.H - Library for accessing X3F Files.
 *
 * Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_DUMP_H
#define X3F_DUMP_H

#include "x3f_io.h"

extern x3f_return_t x3f_dump_raw_data(x3f_t *x3f, char *outfilename);
extern x3f_return_t x3f_dump_jpeg(x3f_t *x3f, char *outfilename);

#endif
