#include <iostream>
#include <inttypes.h>


#include "x3f_denoise_utils.h"
#include "x3f_denoise.h"
#include <algorithm> //for std::sort for the median filter

using namespace cv;


//This nlm denoising includes both a median filtering technique (presumably for hot pixels?)
//and a low frequency technique for reducing larger color blotches.
static void denoise_nlm(Mat& img, float h)
{
  UMat out, sub, sub_dn, sub_res, res;
  float h1[3] = {0.0, h, h}, h2[3] = {0.0, h/8, h/4};
  std::cout << "BEGIN denoising\n";
  fastNlMeansDenoisingAbs(img, out, std::vector<float>(h1, h1+3), 3, 11);
  std::cout << "END denoising\n";
  
  std::cout << "BEGIN V median filtering\n";
  UMat V(out.size(), CV_16U);
  int get_V[2] = { 2,0 }, set_V[2] = { 0,2 };
  mixChannels(std::vector<UMat>(1, out), std::vector<UMat>(2, V), get_V, 1);
  medianBlur(V, V, 3);
  mixChannels(std::vector<UMat>(1, V), std::vector<UMat>(2, out), set_V, 1);
  std::cout << "END V median filtering\n";
  
  std::cout << "BEGIN low-frequency denoising\n";
  resize(out, sub, Size(), 1.0/4, 1.0/4, INTER_AREA);
  fastNlMeansDenoisingAbs(sub, sub_dn, std::vector<float>(h2, h2+3), 3, 21);
  subtract(sub, sub_dn, sub_res, noArray(), CV_16S);
  resize(sub_res, res, out.size(), 0.0, 0.0, INTER_CUBIC);
  subtract(out, res, out, noArray(), CV_16U);
  std::cout << "END low-frequency denoising\n";
  out.copyTo(img);
}


void x3f_denoise(x3f_area16_t *image, x3f_denoise_type_t type)
{
  assert(image->channels == 3);
  assert(type < sizeof(denoise_types)/sizeof(denoise_desc_t));
  const denoise_desc_t *d = &denoise_types[type];
  d->BMT_to_YUV(image);
  Mat img(image->rows, image->columns, CV_16UC3,
          image->data, sizeof(uint16_t)*image->row_stride);
  denoise_nlm(img, d->h);
  d->YUV_to_BMT(image);
}

// NOTE: active has to be a subaera of image, i.e. they have to share
// the same data area.
// NOTE: image, active and qtop will be destructively modified in place.
// NOTE: this function joins both noise reduction and expansion of the lower
// layers, which may need to be functionality that is made more explicit
// for different ISOs
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
    denoise_nlm(act, d->h);  //should be a generic call to noise reduction, so we can swap out here
  }
  resize(img, exp, exp.size(), 0.0, 0.0, INTER_CUBIC);
  qt *= 4;
  int from_to[] = { 0,0 };
  mixChannels(&qt, 1, &exp, 1, from_to, 1);
  if (active_exp) {
    assert(active_exp->channels == 3);
    Mat act_exp(active_exp->rows, active_exp->columns, CV_16UC3,
                active_exp->data, sizeof(uint16_t)*active_exp->row_stride);
    UMat out;
    float h[3] = {0.0, d->h, d->h*2};
    std::cout << "BEGIN Quattro full-resolution denoising\n";
    //again, should be a generic call, so that swap-out can happen more readily
    fastNlMeansDenoisingAbs(act_exp, out, std::vector<float>(h, h+3), 3, 11);
    std::cout << "END Quattro full-resolution denoising\n";
    out.copyTo(act_exp);
  }
  d->YUV_to_BMT(expanded);
}

//arguably a utility function for denoising and other processing, but I'll leave it here for now
void x3f_set_use_opencl(int flag)
{
  ocl::setUseOpenCL(flag);
  if (flag) {
    if (ocl::useOpenCL()) {
      ocl::Device dev = ocl::Device::getDefault();
      std::cout << "OpenCL device name: " << dev.name() << "\n";
      std::cout << "OpenCL device version: " << dev.version() << "\n";
    }
    else std::cerr << "WARNING: OpenCL is not available\n";
  }
  else std::cout << "OpenCL is disabled\n";
}
