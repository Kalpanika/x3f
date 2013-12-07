#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
  int32_t i32 = atoi(argv[1]);
  uint16_t u16 = (uint16_t) i32;
  int32_t ii32 = (int32_t) u16;

  printf("i32: %d u16: %u ii32: %d\n", i32, u16, ii32);

  return 0;
}
