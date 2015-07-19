#ifndef X3F_PROCESS_H
#define X3F_PROCESS_H

#include "x3f_io.h"

typedef enum x3f_color_encoding_e {
  NONE=0,		 /* Preprocessed but unconverted data */
  SRGB=1,		 /* Preproccesed and convered to sRGB */
  ARGB=2,		 /* Preproccesed and convered to Adobee RGB */
  PPRGB=3,		 /* Preproccesed and convered to ProPhoto RGB */
  UNPROCESSED=4,	 /* RAW data without any preprocessing */
  QTOP=5,		 /* Quattro top layer without any preprocessing */
} x3f_color_encoding_t;

typedef struct {
  double black[3];
  uint32_t white[3];
} x3f_image_levels_t;

extern int x3f_get_gain(x3f_t *x3f, char *wb, double *gain);
extern int x3f_get_bmt_to_xyz(x3f_t *x3f, char *wb, double *bmt_to_xyz);
extern int x3f_get_raw_to_xyz(x3f_t *x3f, char *wb, double *raw_to_xyz);

extern int x3f_get_image(x3f_t *x3f,
			 x3f_area16_t *image,
			 x3f_image_levels_t *ilevels,
			 x3f_color_encoding_t encoding,
			 int crop,
			 int denoise,
			 char *wb);

#endif
