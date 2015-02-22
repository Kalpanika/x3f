#include <iostream>
#include <inttypes.h>

#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/imgproc.hpp>

#include "x3f_denoise.h"
#include "x3f_io.h"
#include <algorithm> //for std::sort for the median filter

//#define NLM //turn this off to get aniso denoising

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
  fastNlMeansDenoisingAbs(in, out, h, 3, 11);
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


#ifndef NLM

//need a median filter to remove the pepper noise
//there are faster ways to do this, but this variant is immediately implementable in gl
//probably a variant in opencv for this
static void median_filter(x3f_area16_t *image)
{
  uint32_t x, y, i, j;
  int dx, dy; //needs to be signed, derp
  const uint32_t rows = image->rows;
  const uint32_t cols = image->columns;
  const uint32_t edgeless_cols = cols - 1;
  const uint32_t edgeless_rows = rows - 1;
  const uint32_t jump = image->channels;
  const uint32_t stride = image->row_stride;
  
  uint16_t* current_set = new uint16_t[9];
  memset(current_set, 0, sizeof(uint16_t)*9);
  uint16_t* current_ptr;
  uint16_t median, current_val;
  uint16_t thresh = 2000;
  uint16_t background_thresh = 100;
  //gonna do median filtering by channel for now, to start with.
  uint16_t* scratch = new uint16_t[rows * cols * jump];
  float avg;
  
  
  //memcpy is faster, but it's buggy right now
  //for (y = 1; y < edgeless_rows; y++){
  //  memcpy(scratch + y * stride + jump, image->data + y*stride + jump, edgeless_cols * jump * sizeof(uint16_t));
  //}
  
  for (y = 1; y < edgeless_rows; y++){
    for (x = 1; x < edgeless_cols; x++) {
      for (i = 0; i < jump; i++){
        scratch[y*stride + x*jump + i] = image->data[y*stride + x*jump + i];
      }
    }
  }
  
  for (y = 1; y < edgeless_rows; y++){
    for (x = 1; x < edgeless_cols; x++) {
      for (i = 0; i < jump; i++){
        current_ptr = current_set;
        current_val = image->data[y*stride + x*jump + i];
        for (dy = -1; dy <= 1; dy++){
          for (dx = -1; dx <= 1; dx++, current_ptr++){
            *current_ptr = image->data[(y + dy)*stride + (x + dx)*jump + i];
          }
        }
        std::sort(current_set, current_set + 9);
        //if the current pixel is very very different from the median
        //then replace.  'very very different' means 'more than 50% of the dynamic
        //range of the image away' or 'zero'
        median = current_set[4];
        if (current_val < background_thresh)
        {
          //replace with the local average in this channel
          //scratch[y*stride + x*jump + i] = median;
          for (j = 0; j < jump; j++){
            avg = 0;
            for (dy = -1; dy <= 1; dy++){
              for (dx = -1; dx <= 1; dx++, current_ptr++){
                if (dx == 0 && dy == 0) continue;
                avg += image->data[(y + dy)*stride + (x + dx)*jump + j];
              }
            }
            scratch[y*stride + x*jump + j] = avg/8.0f; //should be int cast when put back in
          }
        }
        
        //if (x == 2000 && y == 2000){
        //  std::cout << "current: " << image->data[y*stride + x*jump + i] << " median: " << scratch[y*stride + x*jump + i] << std::endl;
        //  for (int j = 0; j < 9; j++){
        //    std::cout << "current " << j << " " << current_set[j] << std::endl;
        //  }
        //}
      }
    }
  }
  //copy from scratch back into the image
  //remember leaving edges as they were
  //the memcpy may be buggy, but it's definitely faster
  //for (y = 1; y < edgeless_rows; y++){
  //  memcpy(image->data + y*stride + jump, scratch + y * stride + jump, edgeless_cols * jump * sizeof(uint16_t));
  //}
  
  for (y = 1; y < edgeless_rows; y++){
    for (x = 1; x < edgeless_cols; x++) {
      for (i = 0; i < jump; i++){
        image->data[y*stride + x*jump + i] = scratch[y*stride + x*jump + i];
      }
    }
  }
  
  
  delete [] scratch;
  delete [] current_set;
}

static inline float determine_pixel_difference(const float& v1_1, const float& v1_2, const float& v1_3,
                                               const float& v2_1, const float& v2_2, const float& v2_3)
{ //assumes 3 channels for now, may be a bad assumption
  //also doing the exp version rather than the other technique
  //since the diff is symmetric, can precalc these and cut down on processing time by a factor of 2
  const float c1 = (v1_1 - v2_1);
  const float c2 = (v1_2 - v2_2);
  const float c3 = (v1_3 - v2_3);
  const float K = 0.0000050f;
  const float l2norm = (c1 * c1 + c2 * c2 + c3 * c3); //should be sqrt'd to be mathy, but it's immediately squared
  return -1.0f * exp(-1.0f * l2norm/K); //not the lack of squaring of l2
  //return -1.0f * exp(-1.0f * l2norm*l2norm/K);  //expensive but better
  //return -1.0f/(1.0f - l2norm*l2norm/K);
}

//crude denoising.  Ignores boundary conditions by being super lazy.
static void denoise_aniso(const uint32_t& in_rows, const uint32_t& in_columns, float* image)
{
  uint32_t x, y, i;
  const uint32_t jump = 3;
  const uint32_t num_rows = in_rows;
  const uint32_t row_stride = in_columns * jump;
  const uint32_t edgeless_rows = num_rows - 1;
  const uint32_t edgeless_cols = in_columns - 1;  //used for boundaries
  float* in_ptr;
  float* out_ptr;
  float* out_image = new float[in_rows * in_columns * jump];
  float coeff_fwd, coeff_bkwd, coeff_up, coeff_down, lambda1, lambda2;
  //do the memcpy to keep the boundaries as they were before processing
  //not ideal, because there will be noise there, but it should avoid the green boundary problem.
  //there are a number of ways to solve boundary problems, but none of them are efficient.
  memcpy(out_image, image, in_rows * in_columns * jump * sizeof(float));
  
  //diffusion for the first channel is different, to prevent color bleed
  float* h = new float[jump];
  h[0] = 5.0f;
  h[1] = 1.0f;
  h[2] = 1.0f;
  
  for (y = 1; y < edgeless_rows; y++){
    for (x = 1,
         out_ptr = (out_image + y * row_stride + jump),
         in_ptr = (image + y * row_stride + jump);
         x < edgeless_cols;
         x++, out_ptr += jump, in_ptr += jump){
      
      coeff_fwd = determine_pixel_difference(*in_ptr, *(in_ptr+1), *(in_ptr+2),
                                             *(in_ptr + jump), *(in_ptr + jump + 1), *(in_ptr + jump + 2));
      coeff_bkwd = determine_pixel_difference(*in_ptr, *(in_ptr+1), *(in_ptr+2),
                                             *(in_ptr - jump), *(in_ptr - jump + 1), *(in_ptr - jump + 2));
      coeff_down = determine_pixel_difference(*in_ptr, *(in_ptr+1), *(in_ptr+2),
                                             *(in_ptr + row_stride), *(in_ptr + row_stride + 1), *(in_ptr + row_stride + 2));
      coeff_up = determine_pixel_difference(*in_ptr, *(in_ptr+1), *(in_ptr+2),
                                            *(in_ptr - row_stride), *(in_ptr - row_stride + 1), *(in_ptr - row_stride + 2));
      
      //note that, right now, this is just the isotropic heat equation, just to see how it does.
      //not badly, it seems.
      //coeff_fwd = coeff_up = coeff_down = coeff_bkwd = -1.0f;
      lambda1 = coeff_fwd + coeff_bkwd + coeff_down + coeff_up;
      lambda2 = fabs(lambda1);
      
      for (i = 0; i < jump; i++)
      {
        //independent channels, just to check
        /*coeff_fwd = determine_pixel_difference(*(in_ptr+i), 0, 0,
                                               *(in_ptr+i + jump), 0, 0);
        coeff_bkwd = determine_pixel_difference(*(in_ptr+i), 0, 0,
                                                *(in_ptr+i - jump), 0, 0);
        coeff_down = determine_pixel_difference(*(in_ptr+i), 0, 0,
                                                *(in_ptr+i + row_stride), 0, 0);
        coeff_up = determine_pixel_difference(*(in_ptr+i), 0, 0,
                                              *(in_ptr+i - row_stride), 0, 0);
        
        lambda1 = coeff_fwd + coeff_bkwd + coeff_down + coeff_up;
        lambda2 = fabs(lambda1);*/
        *(out_ptr + i) = *(in_ptr + i) - (lambda2 * *(in_ptr + i) + coeff_fwd * *(in_ptr + jump + i)
                                           + coeff_bkwd * *(in_ptr - jump + i)
                                           + coeff_down * *(in_ptr + row_stride + i)
                                           + coeff_up * *(in_ptr - row_stride + i))/(h[i] *lambda2);

      }
    }
  }
  
  memcpy(image, out_image, in_rows * in_columns * jump * sizeof(float));
  delete [] out_image;
  delete [] h;
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
      
      //if (y == 1000 && x == 1000){
        //std::cout << "pixels during conversion: " << float(*in_ptr)/65535.0f << " " << float(*(in_ptr+1))/65535.0f << " " << float(*(in_ptr+2))/65535.0f << std::endl;
      //}
      for (i = 0; i < jump; i++, out_ptr++, in_ptr++)
      {
        *out_ptr = (float(*in_ptr)/65535.0f);
      }
      
      //if (y == 1000 && x == 1000){
        //std::cout << "pixels after conversion: " << *(out_ptr-3) << " " << *(out_ptr-2) << " " << *(out_ptr-1) << std::endl;
      //}
    }
  }
  return outImage;
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
#endif //NLM

static const denoise_desc_t denoise_types[] = {
  {120.0, BMT_to_YUV_STD, YUV_to_BMT_STD},
  {140.0, BMT_to_YUV_YisT, YUV_to_BMT_YisT},
  {240.0, BMT_to_YUV_YisT, YUV_to_BMT_YisT},
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
  median_filter(image);
  float* float_image = convert_to_float_image(image);
  for (int i = 0; i < 10; i++){
    denoise_aniso(image->rows, image->columns, float_image);
    std::cout << "iteration: " << i << std::endl;
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
			x3f_area16_t *qtop,
			x3f_area16_t *expanded, x3f_area16_t *active_exp)
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

  resize(img, exp, exp.size(), 0.0, 0.0, INTER_LANCZOS4);

  if (active_exp) {
    assert(active_exp->channels == 3);
    Mat act_exp(active_exp->rows, active_exp->columns, CV_16UC3,
		active_exp->data, sizeof(uint16_t)*active_exp->row_stride);

    qt *= 4;
    int from_to[] = { 0,0 };
    mixChannels(&qt, 1, &exp, 1, from_to, 1);
    qt /= 4;

    std::cout << "BEGIN Quattro full-resolution denoising\n";
    fastNlMeansDenoisingAbs(act_exp, act_exp, d->h*4, 3, 11);
    std::cout << "END Quattro full-resolution denoising\n";
  }

  int from_to[] = { 0,0 };
  mixChannels(&qt, 1, &exp, 1, from_to, 1);

  d->YUV_to_BMT(expanded);
}
