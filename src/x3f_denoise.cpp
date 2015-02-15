#include <iostream>
#include <inttypes.h>

#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>

#include "x3f_denoise.h"
#include "x3f_io.h"

using namespace cv;

static const int32_t O_UV = 32768; // To avoid clipping negative values in U,V

// Matrix used to convert BMT to YUV:
//  0  0  4
//  2  0 -2
//  1 -2  1

// TODO: This code assumes that the actual bit depth in X3F is not
//       more than 14, otherwise clipping would occur.
static void raw_to_YUV(x3f_area_t *image, double *scale)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)(p[0]*scale[0]);
      int32_t M = (int32_t)(p[1]*scale[1]);
      int32_t T = (int32_t)(p[2]*scale[2]);

      int32_t Y =           +4*T;
      int32_t U = +2*B      -2*T;
      int32_t V =   +B -2*M   +T;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
//  1/4  1/2  0
//  1/4  1/4 -1/2
//  1/4  0    0
static void YUV_to_raw(x3f_area_t *image, double *scale)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;

      int32_t B = ( +Y +2*U      ) / 4;
      int32_t M = ( +Y   +U -2*V ) / 4;
      int32_t T = ( +Y           ) / 4;

      p[0] = saturate_cast<uint16_t>((int32_t)(B/scale[0]));
      p[1] = saturate_cast<uint16_t>((int32_t)(M/scale[1]));
      p[2] = saturate_cast<uint16_t>((int32_t)(T/scale[2]));
    }
}

void x3f_denoise(x3f_area_t *image, double *gain)
{
  assert(image->channels == 3);

  // Avoid losing precision when 1.0 > WBGain >= 0.5
  // TODO: This would cause clipping with Quattro.
  static const int32_t S_BMT = 2;
  double scale[3] = {S_BMT*gain[0], S_BMT*gain[1], S_BMT*gain[2]};

  raw_to_YUV(image, scale);

  Mat in(image->rows, image->columns, CV_16UC3,
	 image->data, 2*image->row_stride);
  Mat out;

  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoising(in, out, 200.0, 5, 21);
  std::cout << "END denoising\n";

  int from_to[] = { 1,1, 2,2 };
  mixChannels(&out, 1, &in, 1, from_to, 2); // Discard denoised Y

  YUV_to_raw(image, scale);
}
