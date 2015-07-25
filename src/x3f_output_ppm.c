/* X3F_OUTPUT_PPM.C
 *
 * Library for writing the image as PPM.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_output_ppm.h"
#include "x3f_process.h"

#include <stdio.h>
#include <stdlib.h>

/* Routines for writing big endian ppm data */

static void write_16B(FILE *f_out, uint16_t val)
{
  fputc((val>>8) & 0xff, f_out);
  fputc((val>>0) & 0xff, f_out);
}

/* extern */
x3f_return_t x3f_dump_raw_data_as_ppm(x3f_t *x3f,
				      char *outfilename,
				      x3f_color_encoding_t encoding,
				      int crop,
				      int denoise,
				      char *wb,
				      int binary)
{
  x3f_area16_t image;
  FILE *f_out = fopen(outfilename, "wb");
  int row;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  if (!x3f_get_image(x3f, &image, NULL, encoding, crop, denoise, wb) ||
      image.channels < 3) {
    fclose(f_out);
    return X3F_ARGUMENT_ERROR;
  }

  if (binary)
    fprintf(f_out, "P6\n%d %d\n65535\n", image.columns, image.rows);
  else
    fprintf(f_out, "P3\n%d %d\n65535\n", image.columns, image.rows);

  for (row=0; row < image.rows; row++) {
    int col;

    for (col=0; col < image.columns; col++) {
      int color;

      for (color=0; color < 3; color++) {
	uint16_t val = image.data[image.row_stride*row + image.channels*col + color];
	if (binary)
	  write_16B(f_out, val);
	else
	  fprintf(f_out, "%d ", val);
      }
      if (!binary)
	fprintf(f_out, "\n");
    }
  }

  fclose(f_out);
  free(image.buf);

  return X3F_OK;
}
