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
//  0    0    1
//  2    0   -2
//  1   -2    1
static void BMT_to_YUV_YisT(x3f_area16_t *image)
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
static void YUV_to_BMT_YisT(x3f_area16_t *image)
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
//  1/3  1/3  1/3
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
static void YUV_to_BMT_STD(x3f_area16_t *image)
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

static void denoise(const Mat& in, Mat& out, double h)
{
  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoisingAbs(in, out, h, 5, 11);
  std::cout << "END denoising\n";

  std::cout << "BEGIN low-frequency denoising\n";
  Mat sub, sub_dn, sub_res, res;

  resize(out, sub, Size(), 1.0/4, 1.0/4, INTER_AREA);
  fastNlMeansDenoisingAbs(sub, sub_dn, h/4, 5, 21);
  subtract(sub, sub_dn, sub_res, noArray(), CV_16S);
  resize(sub_res, res, out.size(), 0.0, 0.0, INTER_CUBIC);
  subtract(out, res, out, noArray(), CV_16U);
  std::cout << "END low-frequency denoising\n";
}

static inline float determine_pixel_difference(float* ptr1, float* ptr2)
{ //assumes 3 channels for now, may be a bad assumption
  //also doing the exp version rather than the other technique
  //since the diff is symmetric, can precalc these and cut down on processing time by a factor of 2
  return 1.0f - exp(-1.0f * ((fabs(*ptr1 - *ptr2) + fabs(*(ptr1 + 1) - *(ptr2 + 1)) + fabs(*(ptr1 + 2) - *(ptr2 + 2))))/3.0f);
}

//crude denoising.  Ignores boundary conditions by being super lazy.
static void denoise_aniso(const uint32_t& in_rows, const uint32_t& in_columns, float* image)
{
  uint32_t row, col, i;
  const uint32_t jump = 3;
  const uint32_t num_rows = in_rows;
  const uint32_t row_stride = in_columns * jump;
  const uint32_t edgeless_rows = num_rows - 1;
  const uint32_t num_cols = in_columns;
  const uint32_t edgeless_cols = in_columns - 1;  //used for boundaries
  float* in_ptr;
  float* out_ptr;
  float* out_image = new float[in_rows * in_columns * jump];
  float coeff_fwd, coeff_bkwd, coeff_up, coeff_down;
  float out_val = 0;
  
  for (row=1; row < edgeless_rows; row++)
  {
    for (col=1,  //gonna do this with pointers, because I like them
         in_ptr = (image + row * row_stride + jump),
         out_ptr = (out_image + row * row_stride + jump);
         col < edgeless_cols; col++, in_ptr += jump, out_ptr += jump)
    {
      
      if (row == 1000 && col == 1000){
        float *ptr1 = in_ptr;
        float *ptr2 = in_ptr + jump;
        std::cout << "diff 1: " << float(abs(*ptr1 - *ptr2)) << std::endl;
        std::cout << "ptr1: " << *ptr1 << " ptr2: " << *ptr2 << " diff 2: " << *ptr1 - *ptr2 << std::endl;
        std::cout << "diff 3: " << float(abs(*(ptr1+2) - *(ptr2+2))) << std::endl;
      }
      
      coeff_fwd = determine_pixel_difference(in_ptr, (in_ptr + jump));
      coeff_bkwd = determine_pixel_difference(in_ptr, (in_ptr - jump));
      coeff_up = determine_pixel_difference(in_ptr, (in_ptr - row_stride));
      coeff_down = determine_pixel_difference(in_ptr, (in_ptr + row_stride));
      if (row == 1000 && col == 1000){
        std::cout << "coeff_fwd " << coeff_fwd << std::endl;
        std::cout << "coeff_bkwd " << coeff_bkwd << std::endl;
        std::cout << "coeff_up " << coeff_up << std::endl;
        std::cout << "coeff_down " << coeff_down << std::endl;
        std::cout << "pixels " << *in_ptr << " " << *(in_ptr + 1) << " " << *(in_ptr + 2) << std::endl;
        std::cout << "actual coeff " << coeff_fwd * *(in_ptr + jump) << std::endl;
      }
      for (i = 0; i < jump; i++) //could unroll this, I suppose
      {
        out_val = 4.0f * float(*(in_ptr + i)) - coeff_fwd * float(*(in_ptr + jump + i))
                                              - coeff_bkwd * float(*(in_ptr - jump + i))
                                              - coeff_up * float(*(in_ptr - row_stride + i))
                                              - coeff_down * float(*(in_ptr + row_stride + i));
        *(out_ptr + i) = out_val;
        if (row == 1000 && col == 1000){
          std::cout << "start value: " << *(in_ptr + i) << " actual new value: " << *(out_ptr + i) << std::endl;
        }
      }
    }
  }
  memcpy(image, out_image, in_rows * in_columns * jump * sizeof(float));
  delete [] out_image;
}

static float* convert_to_float_image(x3f_area16_t *image)
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
      
      if (y == 1000 && x == 1000){
        std::cout << "pixels during conversion: " << float(*in_ptr)/65535.0f << " " << float(*(in_ptr+1))/65535.0f << " " << float(*(in_ptr+2))/65535.0f << std::endl;
      }
      for (i = 0; i < jump; i++, out_ptr++, in_ptr++)
      {
        *out_ptr = float(*in_ptr)/65535.0f;
      }
      
      if (y == 1000 && x == 1000){
        std::cout << "pixels after conversion: " << *(out_ptr-3) << " " << *(out_ptr-2) << " " << *(out_ptr-1) << std::endl;
      }
    }
  }
  return out_ptr;
}

static void convert_from_float_image(x3f_area16_t *image, float* in_float_image)
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
      for (i = 0; i < jump; i++, in_ptr++, in_float_ptr++)
      {
        new_val = *in_float_ptr;
        if (new_val < 0) new_val = 0;
        if (new_val > 1.0f) new_val = 1.0f; //clamping
        *in_ptr = new_val * 65535.0f;
      }
    }
  }
}

static const denoise_desc_t denoise_types[] = {
  {120.0, BMT_to_YUV_STD, YUV_to_BMT_STD},
  {120.0, BMT_to_YUV_YisT, YUV_to_BMT_YisT},
  {200.0, BMT_to_YUV_YisT, YUV_to_BMT_YisT},
};

void x3f_denoise(x3f_area16_t *image, x3f_denoise_type_t type)
{
  assert(image->channels == 3);
  assert(type < sizeof(denoise_types)/sizeof(denoise_desc_t));
  const denoise_desc_t *d = &denoise_types[type];

  d->BMT_to_YUV(image);
  
#ifdef NLM
  Mat in(image->rows, image->columns, CV_16UC3,
	 image->data, sizeof(uint16_t)*image->row_stride);
  Mat out;

  denoise(in, out, d->h);
  int from_to[] = { 1,1, 2,2 };
  mixChannels(&out, 1, &in, 1, from_to, 2); // Discard denoised Y
#else
  float* float_image = convert_to_float_image(image);
  for (int i = 0; i < 5; i++){
    denoise_aniso(image->rows, image->columns, float_image);
  }
  convert_from_float_image(image, float_image);
  delete [] float_image;
#endif //NLM
  
  d->YUV_to_BMT(image);

}

// NOTE: active has to be a subaera of image, i.e. they have to share
//       the same data area.
// NOTE: image and active will be destructively modified in place.
void x3f_expand_quattro(x3f_area16_t *image, x3f_area16_t *active,
			x3f_area16_t *qtop, x3f_area16_t *expanded)
{
  assert(image->channels == 3);
  assert(qtop->channels == 1);
  assert(X3F_DENOISE_F23 < sizeof(denoise_types)/sizeof(denoise_desc_t));
  const denoise_desc_t *d = &denoise_types[X3F_DENOISE_F23];

  d->BMT_to_YUV(image);

  Mat img(image->rows, image->columns, CV_16UC3,
	  image->data, sizeof(uint16_t)*image->row_stride);
  Mat qt(qtop->rows, qtop->columns, CV_16U,
	 qtop->data, sizeof(uint16_t)*qtop->row_stride);
  Mat exp(expanded->rows, expanded->columns, CV_16UC3,
	  expanded->data, sizeof(uint16_t)*expanded->row_stride);

  assert(qt.size() == exp.size());

  if (active) {
    assert(active->channels == 3);
    Mat act(active->rows, active->columns, CV_16UC3,
	    active->data, sizeof(uint16_t)*active->row_stride);
    denoise(act, act, d->h);
  }
  
  std::cout << "BEGIN Quattro expansion\n";
  resize(img, exp, exp.size(), 0.0, 0.0, INTER_CUBIC);
  int from_to[] = { 0,0 };
  mixChannels(&qt, 1, &exp, 1, from_to, 1);
  std::cout << "END Quattro expansion\n";
  
  d->YUV_to_BMT(expanded);
}
