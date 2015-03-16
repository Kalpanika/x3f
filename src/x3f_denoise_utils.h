#ifndef DENOISE_UTILS
#define DENOISE_UTILS
/* this file contains the support functions for noise reduction algorithms,
   including color conversions and conversions to/from float type images
 */

#ifndef __cplusplus
#error This file can only be included from C++
#endif

#include "x3f_io.h"

#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/imgproc.hpp>

typedef void (*conv_t)(x3f_area16_t *image);


typedef struct {
  float h;
  conv_t BMT_to_YUV;
  conv_t YUV_to_BMT;
} denoise_desc_t;



void BMT_to_YUV_YisT(x3f_area16_t *image);
void YUV_to_BMT_YisT(x3f_area16_t *image);
void BMT_to_YUV_STD(x3f_area16_t *image);
void YUV_to_BMT_STD(x3f_area16_t *image);
void BMT_to_YUV_Yis4T(x3f_area16_t *image);
void YUV_to_BMT_Yis4T(x3f_area16_t *image);


float* convert_to_float_image(x3f_area16_t *image);
void convert_from_float_image(x3f_area16_t *image, float* in_float_image);


const denoise_desc_t denoise_types[] = {
  {100.0, BMT_to_YUV_STD, YUV_to_BMT_STD},
  {70.0, BMT_to_YUV_YisT, YUV_to_BMT_YisT},
  {300.0, BMT_to_YUV_Yis4T, YUV_to_BMT_Yis4T},
};

#endif //DENOISE_UTILS
