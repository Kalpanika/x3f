#include <math.h>
#include <stdio.h>
#include "x3f_matrix.h"

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

/* Convert a 3x1 matrix to a 3x3 diagonal matrix */
void x3f_3x3_diag(double *a, double *b)
{
  M00(b) = M0(a); M01(b) = 0.0;   M02(b) = 0.0;
  M10(b) = 0.0;   M11(b) = M1(a); M12(b) = 0.0;
  M20(b) = 0.0;   M21(b) = 0.0;   M22(b) = M2(a);
}

/* Print a 3x1 matrix */
void x3f_3x1_print(double *a)
{
  printf("%10g\n", M0(a));
  printf("%10g\n", M1(a));
  printf("%10g\n", M2(a));
}

/* Print a 3x3 matrix */
void x3f_3x3_print(double *a)
{
  printf("%10g %10g %10g\n", M00(a), M01(a), M02(a));
  printf("%10g %10g %10g\n", M10(a), M11(a), M12(a));
  printf("%10g %10g %10g\n", M20(a), M21(a), M22(a));
}

/* Conversion matrices for color spaces. NOTE - the result is, of
   course, linear and needs som gamma encoding or similar.  */

/* http://www.color.org/ROMMRGB.pdf */
/* http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space */
void x3f_XYZ_to_ProPhotoRGB(double *a)
{
  M00(a) = +1.3460; M01(a) = -0.2556; M02(a) = -0.0511;
  M10(a) = -0.5446; M11(a) = +1.5082; M12(a) = +0.0205;
  M20(a) = +0.0000; M21(a) = +0.0000; M22(a) = +1.2123;
}

/* http://en.wikipedia.org/wiki/Adobe_RGB_color_space */
/* http://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf */
void x3f_XYZ_to_AdobeRGB(double *a)
{
  M00(a) = +1.96253; M01(a) = -0.61068; M02(a) = -0.34137;
  M10(a) = -0.97876; M11(a) = +1.91615; M12(a) = +0.03342;
  M20(a) = +0.02829; M21(a) = -0.14067; M22(a) = +1.34926;
}

void x3f_AdobeRGB_to_XYZ(double *a)
{
  M00(a) = 0.60974; M01(a) = 0.20528; M02(a) = 0.14919;
  M10(a) = 0.31111; M11(a) = 0.62567; M12(a) = 0.06322;
  M20(a) = 0.01947; M21(a) = 0.06087; M22(a) = 0.74457;
}

/* http://en.wikipedia.org/wiki/SRGB */
void x3f_XYZ_to_sRGB(double *a)
{
  M00(a) = +3.2406; M01(a) = -1.5372; M02(a) = -0.4986;
  M10(a) = -0.9689; M11(a) = +1.8758; M12(a) = +0.0415;
  M20(a) = +0.0557; M21(a) = -0.2040; M22(a) = +1.0570;
}

/* http://en.wikipedia.org/wiki/CIE_1931_color_space */
void x3f_CIERGB_to_XYZ(double *a)
{
  M00(a) = 0.49    ; M01(a) = 0.31    ; M02(a) = 0.20    ;
  M10(a) = 0.17697 ; M11(a) = 0.81240 ; M12(a) = 0.01063 ;
  M20(a) = 0.00    ; M21(a) = 0.01    ; M22(a) = 0.99    ;
}

#define B21 0.17697
void x3f_CIERGB_to_XYZ_strange(double *a)
{
  M00(a) = 0.49    /B21; M01(a) = 0.31    /B21; M02(a) = 0.20    /B21;
  M10(a) = 0.17697 /B21; M11(a) = 0.81240 /B21; M12(a) = 0.01063 /B21;
  M20(a) = 0.00    /B21; M21(a) = 0.01    /B21; M22(a) = 0.99    /B21;
}

/* http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html */
void x3f_Bradford_D50_to_D65(double *a)
{
  M00(a) = +0.9555766 ; M01(a) = -0.0094156; M02(a) = +0.0631636;
  M10(a) = -0.0282895 ; M11(a) = +1.0099416; M12(a) = +0.0210077;
  M20(a) = +0.0122982 ; M21(a) = -0.0204830; M22(a) = +1.3299098;
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

    srgb = round (srgb * max);

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
    gam = round (gam * max);

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
    return (uint16_t)lut[0];
  else if (i>=(size - 1))
    return (uint16_t)lut[size-1];
  else
    return (uint16_t)(lut[i] + frac*(lut[i+1] - lut[i]));
}
