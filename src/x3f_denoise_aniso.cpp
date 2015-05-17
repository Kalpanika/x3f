#include <iostream>
#include "x3f_denoise_aniso.h"
#include "x3f_printf.h"
#include <algorithm> //for std::sort for the median filter

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
  //return -1.0f * exp(-1.0f * l2norm*l2norm/K); //expensive but better
  //return -1.0f/(1.0f - l2norm*l2norm/K);
}

//need a median filter to remove the pepper noise
//there are faster ways to do this, but this variant is immediately implementable in gl
//probably a variant in opencv for this
void median_filter(x3f_area16_t *image)
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
  float median, current_val; //so we can have negatives
  uint16_t thresh = 1000;
  uint16_t background_thresh = 100;  //entirely arbitrarily chosen
  //gonna do median filtering by channel for now, to start with.
  uint16_t* scratch = new uint16_t[rows * cols * jump];
  float avg;

  memcpy(scratch, image->data, rows*cols*jump*sizeof(uint16_t));

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
        if (fabs(current_val - median) > thresh || current_val < background_thresh)
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
      }
    }
  }
  //copy from scratch back into the image
  memcpy(image->data, scratch, rows*cols*jump*sizeof(uint16_t));


  delete [] scratch;
  delete [] current_set;
}

//crude denoising.  Ignores boundary conditions by being super lazy.
void denoise_aniso_float(const uint32_t& in_rows, const uint32_t& in_columns, float* image)
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

      //uncomment out this line to do isotropic noise reduction
      //coeff_fwd = coeff_up = coeff_down = coeff_bkwd = -1.0f;
      lambda1 = coeff_fwd + coeff_bkwd + coeff_down + coeff_up;
      lambda2 = fabs(lambda1);

      for (i = 0; i < jump; i++)
	{
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

//puts the aniso denoising into float format, does the denoising, then
//converts back to shorts.  Along the way, ditches the luminosity channel in the back conversion.
void denoise_aniso(x3f_area16_t *image, const int& in_iterations)
{
  median_filter(image);
  float* float_image = convert_to_float_image(image);
  for (int i = 0; i < in_iterations; i++){
    denoise_aniso_float(image->rows, image->columns, float_image);
    x3f_printf(DEBUG, "iteration: %d\n", i);
  }
  convert_from_float_image(image, float_image);
  delete [] float_image;
}


//even cruder denoising.  Ignores boundary conditions by being super lazy.
//also, is basically a gaussian blur.
void denoise_iso_float(const uint32_t& in_rows, const uint32_t& in_columns, float* image)
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

  coeff_fwd = coeff_up = coeff_down = coeff_bkwd = -1.0f;
  lambda1 = coeff_fwd + coeff_bkwd + coeff_down + coeff_up;
  lambda2 = fabs(lambda1);

  for (y = 1; y < edgeless_rows; y++){
    for (x = 1,
         out_ptr = (out_image + y * row_stride + jump),
         in_ptr = (image + y * row_stride + jump);
         x < edgeless_cols;
         x++, out_ptr += jump, in_ptr += jump){


      for (i = 0; i < jump; i++)
      {
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

//puts the iso denoising into float format, does the denoising, then
//converts back to shorts.  Along the way, ditches the luminosity channel in the back conversion.
void denoise_iso(x3f_area16_t *image, const int& in_iterations)
{
  median_filter(image);
  float* float_image = convert_to_float_image(image);
  for (int i = 0; i < in_iterations; i++){
    denoise_iso_float(image->rows, image->columns, float_image);
    x3f_printf(DEBUG, "iteration: %d\n", i);
  }
  convert_from_float_image(image, float_image);
  delete [] float_image;
}

//use these two functions as function pointers for the dilate/erosion operations
inline uint16_t give_max(const uint16_t& left, const uint16_t& right)
{
  return left > right ? left : right;
}

inline uint16_t give_min(const uint16_t& left, const uint16_t& right)
{
  return left < right ? left : right;
}

//denoising using the morphological operations that compose the 'splotchify' algorithm
//ideally directly implementable in opencv, come to think.
void morphological_op(x3f_area16_t *image, const int& in_radius, const bool& erode)
{
  const int ysize = image->rows;  //forcing a cast to signed for boundary conditions
  const int xsize = image->columns;

  uint16_t (*determinant)(const uint16_t&, const uint16_t&);
  determinant = &give_max;
  uint16_t seed = 0;
  if (erode){
    determinant = &give_min;
    seed = 65535;
  }

  const int the_edge = in_radius*2 + 1;
  bool** theMask = new bool*[the_edge];
  const int sqrRad = in_radius*in_radius;

  int x, y;
  for (y = -in_radius; y <= in_radius; y++){
    theMask[y + in_radius] = new bool[the_edge];
    for (x = -in_radius; x <= in_radius; x++){
      if (x*x + y*y <= sqrRad){
        theMask[y+in_radius][x+in_radius] = true;
      } else {
        theMask[y+in_radius][x+in_radius] = false;
      }
    }
  }

  int dx, dy, i;
  const int jump = image->channels;
  uint16_t* outPixelData = new uint16_t[ysize*xsize*jump];
  memcpy(outPixelData, image->data, ysize*xsize*jump*sizeof(uint16_t));
  uint16_t* in_pxl_ptr = image->data;
  uint16_t* out_pxl_ptr = outPixelData;
  uint16_t* maxes = new uint16_t[jump];
  for (y = 0; y < ysize; y++){
    for (x = 0; x < xsize; x++, in_pxl_ptr+=jump, out_pxl_ptr+=jump){
      for (i = 0; i < jump; i++){
        *(maxes+i) = seed;
      }
      for (dy = -in_radius; dy <= in_radius; dy++){
        if (y + dy < 0 || y + dy >= ysize) continue;
        for (dx = -in_radius; dx <= in_radius; dx++){
          if (x + dx < 0 || x + dx >= xsize) continue;
          if (!theMask[dy+in_radius][dx+in_radius]) continue;
          for (i = 0; i < jump; i++){
            *(maxes + i) = determinant(*(outPixelData + i), *(maxes + i));
          }
        }
      }
      for (i = 0; i < jump; i++){
        *(in_pxl_ptr + i) = *(maxes + i);
      }
    }
  }

  //memcpy(image->data, outPixelData, ysize*xsize*jump*sizeof(uint16_t));
  delete [] outPixelData;
  delete [] maxes;

  for (y = -in_radius; y <= in_radius; y++){
    delete [] theMask[y + in_radius];
  }
  delete [] theMask;
}

//uses morphological operations for every aggressive noise reduction for high ISO images.
//the effect is somewhat artistic (and probably pretty useless) at low ISOs.
void denoise_splotchify(x3f_area16_t *image, const int& in_radius)
{
  size_t i;
  size_t x, y;
  const size_t xsize = image->columns;
  const size_t ysize = image->rows;
  const size_t channel_size = xsize*ysize;
  uint16_t* channel = new uint16_t[channel_size];
  uint16_t* chan_ptr = channel;
  uint16_t* image_ptr;

  int morph_elem = 2; //ellipse
  int morph_size = in_radius;

  cv::Mat element = cv::getStructuringElement( morph_elem, cv::Size( 2*morph_size + 1, 2*morph_size+1 ), cv::Point( morph_size, morph_size ) );
  for (i = 0; i < image->channels; i++){
    //first, get the data, one channel at a time.
    for (y = 0; y < ysize; y++){
      for (x = 0, chan_ptr = &(channel[y*xsize]), image_ptr = &(image->data[y*image->row_stride + i]);
           x < xsize; x++, chan_ptr++, image_ptr+=3){
        *chan_ptr = *image_ptr;
      }
    }

    if (i>0){//skipping the first channel
      cv::Mat img(image->rows, image->columns, CV_16U, //apparently doesn't work on color images
              channel, sizeof(uint16_t)*xsize);

      cv::UMat out;
      cv::morphologyEx( img, out, 3, element );
      cv::morphologyEx( out, img, 2, element );


      for (y = 0; y < ysize; y++){
        for (x = 0, chan_ptr = &(channel[y*xsize]), image_ptr = &(image->data[y*image->row_stride + i]);
             x < xsize; x++, chan_ptr++, image_ptr+=3){
          *image_ptr = *chan_ptr;
        }
      }
    }
  }
  delete [] channel;

}
