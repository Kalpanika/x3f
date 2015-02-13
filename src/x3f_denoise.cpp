#include <iostream>
#include <inttypes.h>

#include <opencv2/core.hpp>
#include <opencv2/photo.hpp>

#include "x3f_denoise.h"
#include "x3f_io.h"

using namespace cv;

void x3f_denoise(x3f_area_t *image)
{
  Mat in(image->rows, image->columns, CV_16UC3,
	 image->data, 2*image->row_stride);
  Mat out;

  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoising(in, out, 5.0, 7, 21);
  std::cout << "END denoising\n";
    
  out.copyTo(in);
}
