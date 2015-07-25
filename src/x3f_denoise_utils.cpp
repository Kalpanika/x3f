/* X3F_DENOISE_UTILS.CPP
 *
 * Library for support functions for noise reduction algorithms,
 * including color conversions and conversions to/from float type images
 *
 * Copyright 2015 - Mark Roden and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include <iostream>
#include <inttypes.h>

#include "x3f_denoise_utils.h"

using namespace cv;

static const int32_t O_UV = 32768; // To avoid clipping negative values in U,V

/* This file contains support functions for matrix transformations,
   etc, used in denoising images.  It is not the denoising functions themselves.
*/

// Matrix used to convert BMT to YUV:
//  0    0    1
//  2    0   -2
//  1   -2    1
void BMT_to_YUV_YisT(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];

      int32_t Y =               +T;
      int32_t U =   +2*B      -2*T;
      int32_t V =     +B -2*M   +T;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
//  1    1/2  0
//  1    1/4 -1/2
//  1    0    0
void YUV_to_BMT_YisT(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;

      int32_t B = ( +2*Y   +U      ) / 2;
      int32_t M = ( +4*Y   +U -2*V ) / 4;
      int32_t T = (   +Y           )    ;

      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

// Matrix used to convert BMT to YUV:
// 0 0 4
// 2 0 -2
// 1 -2 1
void BMT_to_YUV_Yis4T(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];
      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];
      
      int32_t Y = +4*T;
      int32_t U = +2*B -2*T;
      int32_t V = +B -2*M +T;
      
      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
// 1/4 1/2 0
// 1/4 1/4 -1/2
// 1/4 0 0
void YUV_to_BMT_Yis4T(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];
      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;
      
      int32_t B = ( +Y +2*U + 2 ) / 4;
      int32_t M = ( +Y +U -2*V + 2 ) / 4;
      int32_t T = ( +Y + 2 ) / 4;
      
      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

// Matrix used to convert BMT to YUV:
//  1/3  1/3  1/3
//  2    0   -2
//  1   -2    1
void BMT_to_YUV_STD(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t B = (int32_t)p[0];
      int32_t M = (int32_t)p[1];
      int32_t T = (int32_t)p[2];

      int32_t Y = (   +B   +M   +T ) / 3;
      int32_t U =   +2*B      -2*T;
      int32_t V =     +B -2*M   +T;

      p[0] = saturate_cast<uint16_t>(Y);
      p[1] = saturate_cast<uint16_t>(U + O_UV);
      p[2] = saturate_cast<uint16_t>(V + O_UV);
    }
}

// Matrix used to convert YUV to BMT:
//  1    1/4  1/6
//  1    0   -1/3
//  1   -1/4  1/6
void YUV_to_BMT_STD(x3f_area16_t *image)
{
  for (uint32_t row=0; row < image->rows; row++)
    for (uint32_t col=0; col < image->columns; col++) {
      uint16_t *p = &image->data[row*image->row_stride + col*image->channels];

      int32_t Y = (int32_t)p[0];
      int32_t U = (int32_t)p[1] - O_UV;
      int32_t V = (int32_t)p[2] - O_UV;

      int32_t B = ( +12*Y +3*U +2*V ) / 12;
      int32_t M = (  +3*Y        -V ) /  3;
      int32_t T = ( +12*Y -3*U +2*V ) / 12;

      p[0] = saturate_cast<uint16_t>(B);
      p[1] = saturate_cast<uint16_t>(M);
      p[2] = saturate_cast<uint16_t>(T);
    }
}

float* convert_to_float_image(x3f_area16_t *image)
{
  uint32_t x, y, i;
  const uint32_t num_rows = image->rows;
  const uint32_t num_cols = image->columns;
  const uint32_t jump = image->channels;
  float* outImage = new float[num_cols * num_rows * jump];
  float* out_ptr = outImage;
  uint16_t* in_ptr;
  
  for (y = 0; y < num_rows; y++)
    {
      for (x = 0, in_ptr = (image->data + y * image->row_stride); x < num_cols; x++)
	{
	  for (i = 0; i < jump; i++, out_ptr++, in_ptr++)
	    {
	      *out_ptr = (float(*in_ptr)/65535.0f);
	    }
	}
    }
  return outImage;
}

void convert_from_float_image(x3f_area16_t *image, float* in_float_image)
{
  uint32_t x, y, i;
  const uint32_t num_rows = image->rows;
  const uint32_t num_cols = image->columns;
  const uint32_t jump = image->channels;
  float* in_float_ptr = in_float_image;
  uint16_t* in_ptr;
  float new_val;
  for (y = 0; y < num_rows; y++)
    {
      for (x = 0, in_ptr = (image->data + y * image->row_stride); x < num_cols; x++)
	{
	  //skipping the first channel, leaving that behind
	  in_ptr++;
	  in_float_ptr++;
	  for (i = 1; i < jump; i++, in_ptr++, in_float_ptr++)
	    {
	      new_val = *in_float_ptr;
	      if (new_val < 0) new_val = 0;
	      if (new_val > 1.0f) new_val = 1.0f; //clamping
	      *in_ptr = new_val * 65535.0f;
	    }
	}
    }
}
