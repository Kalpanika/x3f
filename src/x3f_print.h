/* X3F_PRINT.H - Library for accessing X3F Files.
 *
 * Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_PRINT_H
#define X3F_PRINT_H

#include "x3f_io.h"

extern uint32_t max_printed_matrix_elements;

extern void x3f_print(x3f_t *x3f);
extern x3f_return_t x3f_dump_meta_data(x3f_t *x3f, char *outfilename);

#endif
