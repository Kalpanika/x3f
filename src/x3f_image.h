/* X3F_IMAGE.H - Library for accessing X3F Files.
 *
 * Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_IMAGE_H
#define X3F_IMAGE_H

#include "x3f_io.h"

extern int x3f_image_area(x3f_t *x3f, x3f_area16_t *image);
extern int x3f_image_area_qtop(x3f_t *x3f, x3f_area16_t *image);
extern int x3f_crop_area(uint32_t *coord, x3f_area16_t *image,
			 x3f_area16_t *crop);
extern int x3f_get_camf_rect(x3f_t *x3f, char *name,
			     x3f_area16_t *image, int rescale,
			     uint32_t *rect);
extern int x3f_crop_area_camf(x3f_t *x3f, char *name,
			      x3f_area16_t *image, int rescale,
			      x3f_area16_t *crop);

#endif
