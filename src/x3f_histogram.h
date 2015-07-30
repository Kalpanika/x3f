/* X3F_HISTOGRAM.H
 *
 * Library for writing histogram data.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_HISTOGRAM_H
#define X3F_HISTOGRAM_H

#include "x3f_io.h"
#include "x3f_process.h"

extern x3f_return_t x3f_dump_raw_data_as_histogram(x3f_t *x3f,
                                                   char *outfilename,
						   x3f_color_encoding_t encoding,
						   int crop,
						   int denoise,
						   int apply_sgain,
						   char *wb,
                                                   int log_hist);

#endif
