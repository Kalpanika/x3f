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

/* Multiply a 3x3 matrix with a 3x1 matrix, giving a 3x1 matrix */
void x3f_3x3_3x1_mul(float *a, float *b, float *c)
{
  M0(c) = M00(a)*M0(b) + M01(a)*M1(b) + M02(a)*M2(b);
  M1(c) = M10(a)*M0(b) + M11(a)*M1(b) + M12(a)*M2(b);
  M2(c) = M20(a)*M0(b) + M21(a)*M1(b) + M22(a)*M2(b);
}

/* Multiply a 3x3 matrix with a 3x3 matrix, giving a 3x3 matrix */
void x3f_3x3_3x3_mul(float *a, float *b, float *c)
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
void x3f_3x3_diag(float *a, float *b)
{
  M00(b) = M0(a); M01(b) = 0.0;   M02(b) = 0.0;
  M10(b) = 0.0;   M11(b) = M1(a); M12(b) = 0.0;
  M20(b) = 0.0;   M21(b) = 0.0;   M22(b) = M2(a);
}

/* Print a 3x1 matrix */
void x3f_3x1_print(float *a)
{
  printf("%10g\n", M0(a));
  printf("%10g\n", M1(a));
  printf("%10g\n", M2(a));
}

/* Print a 3x3 matrix */
void x3f_3x3_print(float *a)
{
  printf("%10g %10g %10g\n", M00(a), M01(a), M02(a));
  printf("%10g %10g %10g\n", M10(a), M11(a), M12(a));
  printf("%10g %10g %10g\n", M20(a), M21(a), M22(a));
}

/* Conversion matrices for color spaces. NOTE - the result is, of
   course, linear and needs som gamma encoding or similar.  */

/* http://www.color.org/ROMMRGB.pdf */
/* http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space */
void x3f_XYZ_to_ProPhotoRGB(float *a)
{
  M00(a) = +1.3460; M01(a) = -0.2556; M02(a) = -0.0511;
  M10(a) = -0.5446; M11(a) = +1.5082; M12(a) = +0.0205;
  M20(a) = +0.0000; M21(a) = +0.0000; M22(a) = +1.2123;
}

/* http://en.wikipedia.org/wiki/Adobe_RGB_color_space */
/* http://www.adobe.com/digitalimag/pdfs/AdobeRGB1998.pdf */
void x3f_XYZ_to_AdobeRGB(float *a)
{
  M00(a) = +1.96253; M01(a) = -0.61068; M02(a) = -0.34137;
  M10(a) = -0.97876; M11(a) = +1.91615; M12(a) = +0.03342;
  M20(a) = +0.02829; M21(a) = -0.14067; M22(a) = +1.34926;
}

/* http://en.wikipedia.org/wiki/SRGB */
void x3f_XYZ_to_sRGB(float *a)
{
  M00(a) = +3.2406; M01(a) = -1.5372; M02(a) = -0.4986;
  M10(a) = -0.9689; M11(a) = +1.8758; M12(a) = +0.0415;
  M20(a) = +0.0557; M21(a) = -0.2040; M22(a) = +1.0570;
}

/* http://en.wikipedia.org/wiki/CIE_1931_color_space */
void x3f_CIERGB_to_XYZ(float *a)
{
  M00(a) = 0.49    ; M01(a) = 0.31    ; M02(a) = 0.20    ;
  M10(a) = 0.17697 ; M11(a) = 0.81240 ; M12(a) = 0.01063 ;
  M20(a) = 0.00    ; M21(a) = 0.01    ; M22(a) = 0.99    ;
}

#define B21 0.17697
void x3f_CIERGB_to_XYZ_strange(float *a)
{
  M00(a) = 0.49    /B21; M01(a) = 0.31    /B21; M02(a) = 0.20    /B21;
  M10(a) = 0.17697 /B21; M11(a) = 0.81240 /B21; M12(a) = 0.01063 /B21;
  M20(a) = 0.00    /B21; M21(a) = 0.01    /B21; M22(a) = 0.99    /B21;
}
