#include <stdio.h>
#include "x3f_matrix.h"

float a[9] = {11.0, 12.0, 13.0,
	      21.0, 22.0, 23.0,
	      31.0, 32.0, 33.0};
float b[9] = {0.0, 0.0, 1.0,
	      0.0, 1.0, 0.0,
	      1.0, 0.0, 0.0};
float x[3] = {1.0,
	      2.0,
	      3.0};


int main(int argc, char *argv[])
{
  float c[9];
  float y[3];
  float diag[9];

  x3f_3x3_3x3_mul(a, b, c);
  x3f_3x3_1x3_mul(c, x, y);
  x3f_3x3_diag(x, diag);

  printf("a\n");
  x3f_3x3_print(a);
  printf("b\n");
  x3f_3x3_print(b);
  printf("c\n");
  x3f_3x3_print(c);

  printf("x\n");
  x3f_1x3_print(x);
  printf("y\n");
  x3f_1x3_print(y);

  printf("diag\n");
  x3f_3x3_print(diag);

  return 0;
}
