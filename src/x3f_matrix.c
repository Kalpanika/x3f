/* X3F_MATRIX.C
 *
 * Library for manipulating matrices.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include <math.h>

#include "x3f_matrix.h"
#include "x3f_printf.h"

#define M0(m) *(m+0)
#define M1(m) *(m+1)
#define M2(m) *(m+2)

#define M00(m) *(m+0)
#define M01(m) *(m+1)
#define M02(m) *(m+2)
#define M10(m) *(m+3)
#define M11(m) *(m+4)
#define M12(m) *(m+5)
#define M20(m) *(m+6)
#define M21(m) *(m+7)
#define M22(m) *(m+8)

void x3f_3x1_invert(double *a, double *ainv)
{
  M0(ainv) = 1.0/M0(a); M1(ainv) = 1.0/M1(a); M2(ainv) = 1.0/M2(a);
}

void x3f_3x1_comp_mul(double *a, double *b, double *c)
{
  M0(c) = M0(a)*M0(b); M1(c) = M1(a)*M1(b); M2(c) = M2(a)*M2(b);
}

/* Multiply a scalar with a 3x1 matrix, giving a 3x1 matrix */
void x3f_scalar_3x1_mul(double a, double *b, double *c)
{
  M0(c) = a*M0(b); M1(c) = a*M1(b); M2(c) = a*M2(b);
}

/* Multiply a scalar with a 3x3 matrix, giving a 3x3 matrix */
void x3f_scalar_3x3_mul(double a, double *b, double *c)
{
  M00(c) = a*M00(b); M01(c) = a*M01(b); M02(c) = a*M02(b);
  M10(c) = a*M10(b); M11(c) = a*M11(b); M12(c) = a*M12(b);
  M20(c) = a*M20(b); M21(c) = a*M21(b); M22(c) = a*M22(b);
}

/* Multiply a 3x3 matrix with a 3x1 matrix, giving a 3x1 matrix */
void x3f_3x3_3x1_mul(double *a, double *b, double *c)
{
  M0(c) = M00(a)*M0(b) + M01(a)*M1(b) + M02(a)*M2(b);
  M1(c) = M10(a)*M0(b) + M11(a)*M1(b) + M12(a)*M2(b);
  M2(c) = M20(a)*M0(b) + M21(a)*M1(b) + M22(a)*M2(b);
}

/* Multiply a 3x3 matrix with a 3x3 matrix, giving a 3x3 matrix */
void x3f_3x3_3x3_mul(double *a, double *b, double *c)
{
  M00(c) = M00(a)*M00(b) + M01(a)*M10(b) + M02(a)*M20(b);
  M01(c) = M00(a)*M01(b) + M01(a)*M11(b) + M02(a)*M21(b);
  M02(c) = M00(a)*M02(b) + M01(a)*M12(b) + M02(a)*M22(b);

  M10(c) = M10(a)*M00(b) + M11(a)*M10(b) + M12(a)*M20(b);
  M11(c) = M10(a)*M01(b) + M11(a)*M11(b) + M12(a)*M21(b);
  M12(c) = M10(a)*M02(b) + M11(a)*M12(b) + M12(a)*M22(b);

  M20(c) = M20(a)*M00(b) + M21(a)*M10(b) + M22(a)*M20(b);
  M21(c) = M20(a)*M01(b) + M21(a)*M11(b) + M22(a)*M21(b);
  M22(c) = M20(a)*M02(b) + M21(a)*M12(b) + M22(a)*M22(b);
}

/* Calculate the inverse of a 3x3 matrix
   http://en.wikipedia.org/wiki/Invertible_matrix */
void x3f_3x3_inverse(double *a, double *ainv)
{
  double A, B, C, D, E, F, G, H, I;
  double det;

  A = +(M11(a)*M22(a)-M12(a)*M21(a));
  B = -(M10(a)*M22(a)-M12(a)*M20(a));
  C = +(M10(a)*M21(a)-M11(a)*M20(a));

  D = -(M01(a)*M22(a)-M02(a)*M21(a));
  E = +(M00(a)*M22(a)-M02(a)*M20(a));
  F = -(M00(a)*M21(a)-M01(a)*M20(a));

  G = +(M01(a)*M12(a)-M02(a)*M11(a));
  H = -(M00(a)*M12(a)-M02(a)*M10(a));
  I = +(M00(a)*M11(a)-M01(a)*M10(a));

  det = M00(a)*A + M01(a)*B + M02(a)*C;

  M00(ainv) = A/det; M01(ainv) = D/det; M02(ainv) = G/det;
  M10(ainv) = B/det; M11(ainv) = E/det; M12(ainv) = H/det;
  M20(ainv) = C/det; M21(ainv) = F/det; M22(ainv) = I/det;
}

/* Convert a 3x1 matrix to a 3x3 diagonal matrix */
void x3f_3x3_diag(double *a, double *b)
{
  M00(b) = M0(a); M01(b) = 0.0;   M02(b) = 0.0;
  M10(b) = 0.0;   M11(b) = M1(a); M12(b) = 0.0;
  M20(b) = 0.0;   M21(b) = 0.0;   M22(b) = M2(a);
}

void x3f_3x3_identity(double *a)
{
  M00(a) = 1.0; M01(a) = 0.0; M02(a) = 0.0;
  M10(a) = 0.0; M11(a) = 1.0; M12(a) = 0.0;
  M20(a) = 0.0; M21(a) = 0.0; M22(a) = 1.0;
}

void x3f_3x3_ones(double *a)
{
  M00(a) = 1.0; M01(a) = 1.0; M02(a) = 1.0;
  M10(a) = 1.0; M11(a) = 1.0; M12(a) = 1.0;
  M20(a) = 1.0; M21(a) = 1.0; M22(a) = 1.0;
}

/* Print a 3x1 matrix */
void x3f_3x1_print(x3f_verbosity_t level, double *a)
{
  x3f_printf(level, "%10g\n", M0(a));
  x3f_printf(level, "%10g\n", M1(a));
  x3f_printf(level, "%10g\n", M2(a));
}

/* Print a 3x3 matrix */
void x3f_3x3_print(x3f_verbosity_t level, double *a)
{
  x3f_printf(level, "%10g %10g %10g\n", M00(a), M01(a), M02(a));
  x3f_printf(level, "%10g %10g %10g\n", M10(a), M11(a), M12(a));
  x3f_printf(level, "%10g %10g %10g\n", M20(a), M21(a), M22(a));
}

/* Conversion matrices for color spaces. NOTE - the result is, of
   course, linear and needs some gamma encoding or similar.  */

/* http://www.color.org/ROMMRGB.pdf */
/* http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space */
void x3f_XYZ_to_ProPhotoRGB(double *a)
{
  M00(a) = +1.3460; M01(a) = -0.2556; M02(a) = -0.0511;
  M10(a) = -0.5446; M11(a) = +1.5082; M12(a) = +0.0205;
  M20(a) = +0.0000; M21(a) = +0.0000; M22(a) = +1.2123;
}

void x3f_ProPhotoRGB_to_XYZ(double *a)
{
  M00(a) = 0.7977; M01(a) = 0.1352; M02(a) = 0.0313;
  M10(a) = 0.2880; M11(a) = 0.7119; M12(a) = 0.0001;
  M20(a) = 0.0000; M21(a) = 0.0000; M22(a) = 0.8249;
}

/* http://en.wikipedia.org/wiki/Adobe_RGB_color_space */
/* http://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf */
void x3f_XYZ_to_AdobeRGB(double *a)
{
  M00(a) = +2.04159; M01(a) = -0.56501; M02(a) = -0.34473;
  M10(a) = -0.96924; M11(a) = +1.87597; M12(a) = +0.04156;
  M20(a) = +0.01344; M21(a) = -0.11836; M22(a) = +1.01517;
}

void x3f_AdobeRGB_to_XYZ(double *a)
{
  M00(a) = 0.57667; M01(a) = 0.18556; M02(a) = 0.18823;
  M10(a) = 0.29737; M11(a) = 0.62736; M12(a) = 0.07529;
  M20(a) = 0.02703; M21(a) = 0.07069; M22(a) = 0.99134;
}

/* http://en.wikipedia.org/wiki/SRGB */
void x3f_XYZ_to_sRGB(double *a)
{
  M00(a) = +3.2406; M01(a) = -1.5372; M02(a) = -0.4986;
  M10(a) = -0.9689; M11(a) = +1.8758; M12(a) = +0.0415;
  M20(a) = +0.0557; M21(a) = -0.2040; M22(a) = +1.0570;
}

void x3f_sRGB_to_XYZ(double *a)
{
  M00(a) = 0.4124; M01(a) = 0.3576; M02(a) = 0.1805;
  M10(a) = 0.2126; M11(a) = 0.7152; M12(a) = 0.0722;
  M20(a) = 0.0193; M21(a) = 0.1192; M22(a) = 0.9505;
}

/* http://en.wikipedia.org/wiki/CIE_1931_color_space */
void x3f_CIERGB_to_XYZ(double *a)
{
  M00(a) = 0.49    ; M01(a) = 0.31    ; M02(a) = 0.20    ;
  M10(a) = 0.17697 ; M11(a) = 0.81240 ; M12(a) = 0.01063 ;
  M20(a) = 0.00    ; M21(a) = 0.01    ; M22(a) = 0.99    ;
}

/* http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html */
void x3f_Bradford_D50_to_D65(double *a)
{
  M00(a) = +0.9555766 ; M01(a) = -0.0230393; M02(a) = +0.0631636;
  M10(a) = -0.0282895 ; M11(a) = +1.0099416; M12(a) = +0.0210077;
  M20(a) = +0.0122982 ; M21(a) = -0.0204830; M22(a) = +1.3299098;
}

void x3f_Bradford_D65_to_D50(double *a)
{
  M00(a) = +1.0478112 ; M01(a) = +0.0228866; M02(a) = -0.0501270;
  M10(a) = +0.0295424 ; M11(a) = +0.9904844; M12(a) = -0.0170491;
  M20(a) = -0.0092345 ; M21(a) = +0.0150436; M22(a) = +0.7521316;
}

void x3f_sRGB_LUT(double *lut, int size, uint16_t max)
{
  double a = 0.055;
  double thres = 0.0031308;
  int i;

  for (i=0; i<size; i++) {
    double lin = (double)i/(size - 1);
    double srgb;

    if (lin <= thres)
      srgb = 12.92 * lin;
    else
      srgb = (1 + a)*pow(lin, 1/2.4) - a;

    srgb *= max;

    if (srgb < 0)
      lut[i] = 0;
    else if (srgb > max)
      lut[i] = max;
    else
      lut[i] = srgb;
  }
}

void x3f_gamma_LUT(double *lut, int size, uint16_t max, double gamma)
{
  int i;

  for (i=0; i<size; i++) {
    double lin = (double)i/(size - 1);
    double gam;

    gam = pow(lin, 1/gamma);
    gam *= max;

    if (gam < 0)
      lut[i] = 0;
    else if (gam > max)
      lut[i] = max;
    else
      lut[i] = gam;
  }
}

uint16_t x3f_LUT_lookup(double *lut, int size, double val)
{
  double index = val*(size - 1);
  int i = (int)floor(index);
  double frac = index - i;

  if (i<0)
    return (uint16_t)round(lut[0]);
  else if (i>=(size - 1))
    return (uint16_t)round(lut[size-1]);
  else
    return (uint16_t)round(lut[i] + frac*(lut[i+1] - lut[i]));
}
