#include <iostream>
#include "x3f_denoise_aniso.h"

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
    std::cout << "iteration: " << i << std::endl;
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
    std::cout << "iteration: " << i << std::endl;
  }
  convert_from_float_image(image, float_image);
  delete [] float_image;
}

//use these two functions as function pointers for the dilate/erosion operations
bool give_max(const uint16_t& left, const uint16_t& right)
{
  return left > right;
}

bool give_min(const uint16_t& left, const uint16_t& right)
{
  return left < right;
}

//denoising using the morphological operations that compose the 'splotchify' algorithm
//ideally directly implementable in opencv, come to think.
void morphological_op(x3f_area16_t *image, const int& in_radius, bool* in_mask, const bool& erode)
{
  const int ysize = image->rows;  //forcing a cast to signed for boundary conditions
  const int xsize = image->columns;
  
  int red = 0;
  int green = 1;
  int blue = 2;
  //int alpha = 3;
  
  bool (*determinant)(const uint16_t&, const uint16_t&);
  determinant = &give_max;
  if (erode){
    determinant = &give_min;
  }
  
  int redMax, blueMax, greenMax;
  int currRed, currBlue, currGreen;
  int index = 0;
  int currIndex;
  int x, y;
  int dx, dy;
  const int the_edge = in_radius*2 + 1;
  const int jump = image->channels;
  uint16_t* outPixelData = new uint16_t[ysize*xsize*jump];
  for (y = 0; y < ysize; y++){
    for (x = 0; x < xsize; x++){
      redMax = blueMax = greenMax = 0;
      for (dy = -in_radius; dy <= in_radius; dy++){
        if (y + dy < 0 || y + dy >= ysize) continue;
        for (dx = -in_radius; dx <= in_radius; dx++){
          if (x + dx < 0 || x + dx >= xsize) continue;
          if (!in_mask[(y+in_radius)* the_edge + x+in_radius]) continue;
          currIndex = ((y+dy)*xsize + (x+dx))*jump;
          currRed = image->data[currIndex + red];
          currGreen = image->data[currIndex+ green];
          currBlue = image->data[currIndex + blue];
          if (determinant(currRed, redMax)) redMax = currRed;
          if (determinant(currBlue, blueMax)) blueMax = currBlue;
          if (determinant(currGreen, greenMax)) greenMax = currGreen;
        }
      }
      index = (y*xsize  + x)*jump;
      outPixelData[index + red] = redMax;
      outPixelData[index + green] = greenMax;
      outPixelData[index + blue] = blueMax;
      //outPixelData[index + alpha] = pixelData[index+alpha];
    }
  }
  
  memcpy(image->data, outPixelData, ysize*xsize*jump*sizeof(uint16_t));
  delete [] outPixelData;
}

//uses morphological operations for every aggressive noise reduction for high ISO images.
//the effect is somewhat artistic (and probably pretty useless) at low ISOs.
void denoise_splotchify(x3f_area16_t *image, const int& in_radius)
{
  int the_edge = in_radius*2 + 1;
  bool* theMask = new bool[the_edge*the_edge];
  int sqrRad = in_radius*in_radius;
  
  int x, y;
  for (y = -in_radius; y <= in_radius; y++){
    for (x = -in_radius; x <= in_radius; x++){
      if (x*x + y*y <= sqrRad){
        theMask[(y+in_radius)* the_edge + x+in_radius] = true;
      } else {
        theMask[(y+in_radius)* the_edge + x+in_radius] = false;
      }
    }
  }
  
  //save the original luminosity channel
  uint16_t* original_data = new uint16_t[image->rows * image->columns * image->channels];
  memcpy(original_data, image->data, image->rows * image->columns * image->channels*sizeof(uint16_t));
  
  //the order of operations is
  //dilate, erode, erode, dilate
  morphological_op(image, in_radius, theMask, false);
  morphological_op(image, in_radius, theMask, true);
  morphological_op(image, in_radius, theMask, true);
  morphological_op(image, in_radius, theMask, false);
  
  delete [] theMask;
  
  //restore luminosity channel
  const int ysize = image->rows; //forcing int cast to remove warning
  const int xsize = image->columns;
  uint16_t* image_ptr = image->data;
  uint16_t* orig_ptr = original_data;
  for (y = 0; y < ysize; y++){
    for (x = 0; x < xsize; x++, image_ptr+=3, orig_ptr+=3){
      *image_ptr = *orig_ptr;
    }
  }
  
  delete [] original_data;
  
}
