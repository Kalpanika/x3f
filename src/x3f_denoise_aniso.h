#ifndef DENOISE_ANISO
#define DENOISE_ANISO

#include "x3f_denoise_utils.h"  //has the C++ error define in it

//but adding it, just in case it's not entirely clear
#ifndef __cplusplus
#error This file can only be included from C++
#endif

void median_filter(x3f_area16_t *image);
void denoise_aniso(x3f_area16_t *image, const int& in_iterations);
void denoise_iso(x3f_area16_t *image, const int& in_iterations);
void denoise_splotchify(x3f_area16_t *image, const int& in_radius);

#endif //DENOISE_ANISO
