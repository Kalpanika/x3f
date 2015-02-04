#include <iostream>
#include <inttypes.h>

using namespace std;

#include "x3f_denoise.h"
#include "x3f_io.h"

#include "NLMeansDenoise.h"

void x3f_denoise(x3f_area_t *image)
{
  uint16_t *in = new uint16_t[image->columns*image->rows];
  uint16_t *out = new uint16_t[image->columns*image->rows];
  uint32_t ch;

  for (ch=0; ch < image->channels; ch++) {
    uint32_t row, col;

    for (row=0; row < image->rows; row++)
      for (col=0; col < image->columns; col++)
	in[row*image->columns + col] =
	  image->data[row*image->row_stride + col*image->channels + ch];

    cout << "Denoising channel: " << ch << endl;
    NonLocalMeansDenoisingByHashPatch(in, out, NULL,
				      image->columns, image->rows,
				      5, 3);

    for (row=0; row < image->rows; row++)
      for (col=0; col < image->columns; col++)
	  image->data[row*image->row_stride + col*image->channels + ch] =
	    out[row*image->columns + col];
  }

  delete[] in;
  delete[] out;
}
