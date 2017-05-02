/* X3F_HISTOGRAM.C
 *
 * Library for writing histogram data.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_histogram.h"
#include "x3f_process.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int ilog(int i, double base, int steps)
{
  if (i <= 0)
    /* Special case as log(0) is not defined. */
    return 0;
  else {
    double log = log10((double)i) / log10(base);

    return (int)(steps * log);
  }
}

static int ilog_inv(int i, double base, int steps)
{
  return (int)round(pow(base, (double)i/steps));
}

#define BASE 2.0
#define STEPS 10

/* extern */
x3f_return_t x3f_dump_raw_data_as_histogram(x3f_t *x3f,
					    char *outfilename,
					    x3f_color_encoding_t encoding,
					    int crop,
					    int fix_bad,
					    int denoise,
					    int apply_sgain,
					    char *wb,
					    int log_hist)
{
  x3f_area16_t image;
  FILE *f_out = fopen(outfilename, "wb");
  uint32_t *histogram[3];
  int color, i;
  int row;
  uint16_t max = 0;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  if (!x3f_get_image(x3f, &image, NULL, encoding,
		     crop, fix_bad, denoise, apply_sgain,
		     wb) ||
      image.channels < 3) {
    fclose(f_out);
    return X3F_ARGUMENT_ERROR;
  }

  for (color=0; color < 3; color++)
    histogram[color] = (uint32_t *)calloc(1<<16, sizeof(uint32_t));

  for (row=0; row < image.rows; row++) {
    int col;

    for (col=0; col < image.columns; col++)
      for (color=0; color < 3; color++) {
	uint16_t val =
	  image.data[image.row_stride*row + image.channels*col + color];

	if (log_hist)
	  val = ilog(val, BASE, STEPS);

	histogram[color][val]++;
	if (val > max) max = val;
      }
  }

  for (i=0; i <= max; i++) {
    uint32_t val[3];

    for (color=0; color < 3; color++)
      val[color] = histogram[color][i];

    if (val[0] || val[1] || val[2]) {
      if (log_hist) {
	fprintf(f_out, "%5d, %5d , %6d , %6d , %6d\n",
		i, ilog_inv(i, BASE, STEPS), val[0], val[1], val[2]);
      } else {
	fprintf(f_out, "%5d , %6d , %6d , %6d\n",
		i, val[0], val[1], val[2]);
      }
    }
  }

  for (color=0; color < 3; color++)
    free(histogram[color]);

  fclose(f_out);
  free(image.buf);

  return X3F_OK;
}
