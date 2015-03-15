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
