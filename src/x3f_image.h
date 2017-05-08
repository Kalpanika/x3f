/* X3F_IMAGE.H
 *
 * Library for access to image data.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_IMAGE_H
#define X3F_IMAGE_H

#include "x3f_io.h"

typedef enum {COL_SIDE_LEFT = 0,
	      COL_SIDE_RIGHT = 1,
	      COL_SIDE_WRONG = 2} col_side_t;

extern int x3f_image_area(x3f_t *x3f, x3f_area16_t *image);
extern int x3f_image_area_qtop(x3f_t *x3f, x3f_area16_t *image);
extern int x3f_crop_area(uint32_t *coord, x3f_area16_t *image,
			 x3f_area16_t *crop);
extern int x3f_crop_area8(uint32_t *coord, x3f_area8_t *image,
			  x3f_area8_t *crop);
extern int x3f_get_camf_rect(x3f_t *x3f, char *name,
			     x3f_area16_t *image, int rescale,
			     uint32_t *rect);
extern int x3f_crop_area_column(x3f_t *x3f, col_side_t which_side,
				x3f_area16_t *image, int rescale,
				x3f_area16_t *crop);
extern int x3f_crop_area_camf(x3f_t *x3f, char *name,
			      x3f_area16_t *image, int rescale,
			      x3f_area16_t *crop);
extern int x3f_crop_area8_camf(x3f_t *x3f, char *name,
			       x3f_area8_t *image, int rescale,
			       x3f_area8_t *crop);

#endif
