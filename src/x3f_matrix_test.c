#include <stdio.h>
#include "x3f_matrix.h"

double a[9] = {11.0, 12.0, 13.0,
	       21.0, 22.0, 23.0,
	       31.0, 32.0, 33.0};
double b[9] = {0.0, 0.0, 1.0,
	       0.0, 1.0, 0.0,
	       1.0, 0.0, 0.0};
double x[3] = {1.0,
	       2.0,
	       3.0};


int main(int argc, char *argv[])
{
  double c[9];
  double y[3];
  double diag[9];
  double argb[9], argb_inv[9], identity[9];
  double srgb[9];
  double pprgb[9];

  x3f_3x3_3x3_mul(a, b, c);
  x3f_3x3_3x1_mul(c, x, y);
  x3f_3x3_diag(x, diag);
  x3f_XYZ_to_AdobeRGB(argb);
  x3f_3x3_inverse(argb, argb_inv);
  x3f_3x3_3x3_mul(argb_inv, argb, identity);
  x3f_XYZ_to_sRGB(srgb);
  x3f_XYZ_to_ProPhotoRGB(pprgb);

  printf("a\n");
  x3f_3x3_print(INFO, a);
  printf("b\n");
  x3f_3x3_print(INFO, b);
  printf("c\n");
  x3f_3x3_print(INFO, c);

  printf("x\n");
  x3f_3x1_print(INFO, x);
  printf("y\n");
  x3f_3x1_print(INFO, y);

  printf("diag\n");
  x3f_3x3_print(INFO, diag);

  printf("Adobe RGB\n");
  x3f_3x3_print(INFO, argb);
  printf("Adobe RGB inverse\n");
  x3f_3x3_print(INFO, argb_inv);
  printf("Identity\n");
  x3f_3x3_print(INFO, identity);
  printf("sRGB\n");
  x3f_3x3_print(INFO, srgb);
  printf("Pro Photo RGB\n");
  x3f_3x3_print(INFO, pprgb);

  return 0;
}
