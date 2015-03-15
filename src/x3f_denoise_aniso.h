#ifndef DENOISE_ANISO
#define DENOISE_ANISO

#include "x3f_denoise_utils.h"

void median_filter(x3f_area16_t *image);
void denoise_aniso(const uint32_t& in_rows, const uint32_t& in_columns, float* image);
void denoise_iso(const uint32_t& in_rows, const uint32_t& in_columns, float* image);
void denoise_splotchify(const uint32_t& in_rows, const uint32_t& in_columns, float* image);

#endif //DENOISE_ANISO
