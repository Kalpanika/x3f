/* X3F_PRINT.H
 *
 * Library for printing meta data found in X3F files.
 *
 * Copyright 2015 - Roland and Erik Karlsson
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
