/* Test program for generating exponential intervals */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

int main(int argc, char *argv[])
{
  double step = 0.5;
  double mult;
  double limit = 12.0;
  double d;
  int i;
  long long int min = 0;

  char *limit_str="-limit=";
  char *step_str="-step=";
  int limit_len = strlen(limit_str);
  int step_len = strlen(step_str);

  for (i=1; i<argc; i++) {
    if (!strncmp(limit_str, argv[i], limit_len))
      limit = atof(&argv[i][limit_len]);
    else if (!strncmp(step_str, argv[i], step_len))
      step = atof(&argv[i][step_len]);
    else {
      fprintf(stderr, "usage: %s [-limit=<LIMIT>] [-step=<STEP>]\n", argv[0]);
      return 1;
    }
  }

  mult = exp2(step);

  printf("step = %g (%g)\n", step, mult);
  printf("limit = %g (%g)\n", limit, exp2(limit));

  for (i = 0, d = 1.0; d <= exp2(limit) + 1; i++, d *= mult) {
    long long int max = (long long int)round(d) - 1;

    if (max >= min) {
      printf("%5.2f: [%lld - %lld (%g)]\n", i*step, min, max, d);
      min = max + 1;
    }
  }

  return 0;
}
