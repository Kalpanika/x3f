/* X3F_MATRIX.H
 *
 * Library for manipulating matrices.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_MATRIX_H
#define X3F_MATRIX_H

#include "x3f_printf.h"

#include <inttypes.h>

extern void x3f_3x1_invert(double *a, double *ainv);
extern void x3f_3x1_comp_mul(double *a, double *b, double *c);

extern void x3f_scalar_3x1_mul(double a, double *b, double *c);
extern void x3f_scalar_3x3_mul(double a, double *b, double *c);

extern void x3f_3x3_3x1_mul(double *a, double *b, double *c);
extern void x3f_3x3_3x3_mul(double *a, double *b, double *c);

extern void x3f_3x3_inverse(double *a, double *ainv);

extern void x3f_3x3_identity(double *a);
extern void x3f_3x3_diag(double *a, double *b);
extern void x3f_3x3_ones(double *a);

extern void x3f_3x1_print(x3f_verbosity_t level, double *a);
extern void x3f_3x3_print(x3f_verbosity_t level, double *a);

extern void x3f_XYZ_to_ProPhotoRGB(double *a);
extern void x3f_ProPhotoRGB_to_XYZ(double *a);

extern void x3f_XYZ_to_AdobeRGB(double *a);
extern void x3f_AdobeRGB_to_XYZ(double *a);

extern void x3f_XYZ_to_sRGB(double *a);
extern void x3f_sRGB_to_XYZ(double *a);

extern void x3f_CIERGB_to_XYZ(double *a);

extern void x3f_Bradford_D50_to_D65(double *a);
extern void x3f_Bradford_D65_to_D50(double *a);

extern void x3f_sRGB_LUT(double *lut, int size, uint16_t max);
extern void x3f_gamma_LUT(double *lut, int size, uint16_t max, double gamma);
extern uint16_t x3f_LUT_lookup(double *lut, int size, double val);

#endif	/* X3F_MATRIX_H */
