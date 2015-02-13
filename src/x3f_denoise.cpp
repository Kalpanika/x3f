#include <iostream>
#include <inttypes.h>

#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>

#include "x3f_denoise.h"
#include "x3f_io.h"

using namespace cv;

static const int32_t OFFSET = 32768; // Avoid clipping negative values in U,V

// Matrix used to convert TBM to YUV:
//  1  1  1
//  2  0 -2
//  1 -2  1

// TODO: This codes assumes that the actual bit depth in X3F is not
//       more than 14, otherwise clipping would occur
static void raw_to_YUV(x3f_area_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];

      int32_t Y =   +T   +B   +M;
      int32_t U = +2*T      -2*M;
      int32_t V =   +T -2*B   +M;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + OFFSET);
      p[2] = saturate_cast<uint16_t>(V + OFFSET);
    }
}

// Matrix used to convert YUV to TBM:
//  1/3  1/4  1/6
//  1/3  0   -1/3
//  1/3 -1/4  1/6

// TODO: This codes assumes that the actual bit depth in X3F is not
//       more than 14, otherwise clipping would occur
static void YUV_to_raw(x3f_area_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - OFFSET;
      int32_t V = (int32_t)p[2] - OFFSET;

      int32_t T = ( +4*Y +3*U +2*V ) / 12;
      int32_t B = ( +4*Y      -4*V ) / 12;
      int32_t M = ( +4*Y -3*U +2*V ) / 12;

      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

void x3f_denoise(x3f_area_t *image)
{
  assert(image->channels == 3);

  raw_to_YUV(image);

  Mat in(image->rows, image->columns, CV_16UC3,
	 image->data, 2*image->row_stride);
  Mat out;

  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoising(in, out, 15.0, 7, 21);
  std::cout << "END denoising\n";
    
  int from_to[] = { 1,1, 2,2 };
  mixChannels(&out, 1, &in, 1, from_to, 2); // Discard denoised Y

  YUV_to_raw(image);
}
