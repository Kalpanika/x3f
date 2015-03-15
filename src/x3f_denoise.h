#ifndef X3F_DENOISE_H
#define X3F_DENOISE_H

#include "x3f_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  X3F_DENOISE_STD=0,
  X3F_DENOISE_F20=1,
  X3F_DENOISE_F23=2,
} x3f_denoise_type_t;

extern void x3f_denoise(x3f_area16_t *image, x3f_denoise_type_t type);
extern void x3f_expand_quattro(x3f_area16_t *image,
			       x3f_area16_t *active,
			       x3f_area16_t *qtop,
			       x3f_area16_t *expanded,
			       x3f_area16_t *active_exp);

extern void x3f_set_use_opencl(int flag);

#ifdef __cplusplus
}
#endif

#endif
