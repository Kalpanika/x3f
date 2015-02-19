#include <iostream>
#include <inttypes.h>

#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/imgproc.hpp>

#include "x3f_denoise.h"
#include "x3f_io.h"

using namespace cv;

typedef void (*conv_t)(x3f_area16_t *image);

typedef struct {
  double h;
  conv_t BMT_to_YUV;
  conv_t YUV_to_BMT;
} denoise_desc_t;

static const int32_t O_UV = 32768; // To avoid clipping negative values in U,V

// Matrix used to convert BMT to YUV:
//  0    0    4
//  2    0   -2
//  1   -2    1
static void BMT_to_YUV_F20(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];

      int32_t Y =             +4*T;
      int32_t U =   +2*B      -2*T;
      int32_t V =     +B -2*M   +T;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
//  1/4  1/2  0
//  1/4  1/4 -1/2
//  1/4  0    0
static void YUV_to_BMT_F20(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;

      int32_t B = (   +Y +2*U      ) / 4;
      int32_t M = (   +Y   +U -2*V ) / 4;
      int32_t T = (   +Y           ) / 4;

      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

// Matrix used to convert BMT to YUV:
//  4/3  4/3  4/3
//  2    0   -2
//  1   -2    1
static void BMT_to_YUV_STD(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];

      int32_t Y = ( +4*B +4*M +4*T ) / 3;
      int32_t U =   +2*B      -2*T;
      int32_t V =     +B -2*M   +T;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
//  1/4  1/4  1/6
//  1/4  0   -1/3
//  1/4 -1/4  1/6
static void YUV_to_BMT_STD(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;

      int32_t B = ( +3*Y +3*U +2*V ) / 12;
      int32_t M = ( +3*Y      -4*V ) / 12;
      int32_t T = ( +3*Y -3*U +2*V ) / 12;

      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

static const denoise_desc_t denoise_types[] = {
  {80.0, BMT_to_YUV_STD, YUV_to_BMT_STD},
  {160.0, BMT_to_YUV_F20, YUV_to_BMT_F20},
};

void x3f_denoise(x3f_area16_t *image, x3f_denoise_type_t type)
{
  assert(image->channels == 3);
  assert(type < sizeof(denoise_types)/sizeof(denoise_desc_t));
  
  const denoise_desc_t *d = &denoise_types[type];

  d->BMT_to_YUV(image);

  Mat in(image->rows, image->columns, CV_16UC3,
	 image->data, 2*image->row_stride);
  Mat out;

  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoisingAbs(in, out, d->h, 5, 11);
  std::cout << "END denoising\n";

  std::cout << "BEGIN low-frequency denoising\n";
  Mat sub, sub_dn, sub_res, res;

  resize(out, sub, Size(), 0.25, 0.25, INTER_AREA);
  fastNlMeansDenoisingAbs(sub, sub_dn, 0.25*d->h, 5, 21);
  subtract(sub, sub_dn, sub_res, noArray(), CV_16S);
  resize(sub_res, res, out.size(), 0.0, 0.0, INTER_CUBIC);
  subtract(out, res, out, noArray(), CV_16U);
  std::cout << "END low-frequency denoising\n";

  int from_to[] = { 1,1, 2,2 };
  mixChannels(&out, 1, &in, 1, from_to, 2); // Discard denoised Y

  d->YUV_to_BMT(image);
}
