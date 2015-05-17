#ifndef X3F_OUTPUT_DNG_H
#define X3F_OUTPUT_DNG_H

#include "x3f_io.h"

extern x3f_return_t x3f_dump_raw_data_as_dng(x3f_t *x3f, char *outfilename,
					     int denoise, char *wb,
					     int compress);

#endif
