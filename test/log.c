/* Test program for generating exponential intervals */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

int ilog(int i, double base, int steps)
{
  if (i <= 0)
    /* Special case as log(0) is not defined. */
    return 0;
  else {
    double log = log10((double)i) / log10(base);

    return (int)(steps * log);
  }
}

int main(int argc, char *argv[])
{
  int max = 4095;
  int steps = 2;

  char *max_str="-max=";
  int max_len = strlen(max_str);

  char *step_str="-steps=";
  int step_len = strlen(step_str);

  int i;
  int prev = -100;

  for (i=1; i<argc; i++) {
    if (!strncmp(max_str, argv[i], max_len))
      max = atoi(&argv[i][max_len]);
    else if (!strncmp(step_str, argv[i], step_len))
      steps = atoi(&argv[i][step_len]);
    else {
      fprintf(stderr, "usage: %s [-max=<MAX>] [-steps=<STEPS>]\n", argv[0]);
      return 1;
    }
  }

  printf("steps = %d\n", steps);
  printf("max = %d\n", max);

  for (i = 0; i <= max; i++) {
    int curr = ilog(i, 2.0, steps);
    if (curr != prev) {
      printf("%d: %d (%g)\n", i, curr, (double)curr/steps);
      prev = curr;
    }
  }

  return 0;
}
