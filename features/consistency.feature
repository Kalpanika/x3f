Feature: the output produced by the converter is stable


Scenario Outline: conversions to various outputs will produce exactly the same images
   Given an input image <image> without a <converted_image>
    when the <image> is converted by the code to <file_type>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | file_type | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | DNG | x3f_test_files/_SDI8040.X3F.dng | efa34925dd4e4425726da74cbae9955b |
| x3f_test_files/_SDI8040.X3F | TIFF | x3f_test_files/_SDI8040.X3F.tif | cb67d12ec0a4fd318a426276f527f1a9 |
| x3f_test_files/_SDI8040.X3F | PPM | x3f_test_files/_SDI8040.X3F.ppm | a2bea89af18bb24efd289b73007b2413 |
| x3f_test_files/_SDI8040.X3F | JPG | x3f_test_files/_SDI8040.X3F.jpg | 357f126f6435345642bfbc6171745d00 |
| x3f_test_files/_SDI8040.X3F | META | x3f_test_files/_SDI8040.X3F.meta | 2e81db66465fa97366e64b943324a514 |
| x3f_test_files/_SDI8040.X3F | RAW | x3f_test_files/_SDI8040.X3F.raw | 89582789c24f86aaefd92395bc4f0572 |
| x3f_test_files/_SDI8040.X3F | PPM-ASCII | x3f_test_files/_SDI8040.X3F.ppm | 74e6652a6a1325949d086bc9f099fbe7 |
| x3f_test_files/_SDI8040.X3F | HISTOGRAM | x3f_test_files/_SDI8040.X3F.csv | 8ad3868b2c7b871555932a783c16397d |
| x3f_test_files/_SDI8040.X3F | LOGHIST | x3f_test_files/_SDI8040.X3F.csv | 0c23a7767659c894b81ba64326517eba |

| x3f_test_files/_SDI8284.X3F | DNG | x3f_test_files/_SDI8284.X3F.dng | 71f56b6bdb9f3e403af2c21d16c76664 |
| x3f_test_files/_SDI8284.X3F | TIFF | x3f_test_files/_SDI8284.X3F.tif | 26199384d894ae723292e5ecc40ad194 |
| x3f_test_files/_SDI8284.X3F | PPM | x3f_test_files/_SDI8284.X3F.ppm | 06c85f54fe3a954d4e564efbbb8d8a8b |
| x3f_test_files/_SDI8284.X3F | JPG | x3f_test_files/_SDI8284.X3F.jpg | 87cd494d3bc4eab4e481de6afeb058de |
| x3f_test_files/_SDI8284.X3F | META | x3f_test_files/_SDI8284.X3F.meta | 0a77d95cf4f53acec52c11e756590a28 |
| x3f_test_files/_SDI8284.X3F | RAW | x3f_test_files/_SDI8284.X3F.raw | b9d230401b9822978b49ec244c2f0668 |
| x3f_test_files/_SDI8284.X3F | PPM-ASCII | x3f_test_files/_SDI8284.X3F.ppm | cf48c36eddbddccefcf44378d2e45ba8 |
| x3f_test_files/_SDI8284.X3F | HISTOGRAM | x3f_test_files/_SDI8284.X3F.csv | 78edd0177ac057d77ed62d560267b640 |
| x3f_test_files/_SDI8284.X3F | LOGHIST | x3f_test_files/_SDI8284.X3F.csv | 9a8f38c290c52858200166282daba772 |


Scenario Outline: conversions to various compressed outputs will produce exactly the same images
   Given an input image <image> without a <converted_image>
    when the <image> is converted and compressed by the code to <file_type>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | file_type | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | DNG | x3f_test_files/_SDI8040.X3F.dng | 00bcb49957164819385b52b48f027b89 |
| x3f_test_files/_SDI8040.X3F | TIFF | x3f_test_files/_SDI8040.X3F.tif | 94e05ac8c234d733fffd5e00ac068203 |

| x3f_test_files/_SDI8284.X3F | DNG | x3f_test_files/_SDI8284.X3F.dng | 151316eb4ed6e1fe982a4a5feda218f0 |
| x3f_test_files/_SDI8284.X3F | TIFF | x3f_test_files/_SDI8284.X3F.tif | 2d71f992245597acc49f80d27f036d27 |


Scenario Outline: denoised conversions to dng will produce the exact same outputs
   Given an input image <image> without a <converted_image>
    when the <image> is denoised and converted by the code
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | x3f_test_files/_SDI8040.X3F.dng | 8d62244e47bbd657587c376331b1a5da |
| x3f_test_files/_SDI8284.X3F | x3f_test_files/_SDI8284.X3F.dng | f0bcd7161a5dd1a671e78d3978a24264 |


Scenario Outline: denoised conversions to tiff will produce the exact same outputs
   Given an input image <image> without a <converted_image>
    when the <image> is denoised and converted by the code to a cropped color TIFF
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | x3f_test_files/_SDI8040.X3F.tif | c15d8761cbcaffd2ab381b9549a31e6b |
| x3f_test_files/_SDI8284.X3F | x3f_test_files/_SDI8284.X3F.tif | 9afe0f0a2e55d38beb2957ec6401ed52 |


Scenario Outline: conversions to tiff will produce the exact same outputs
   Given an input image <image> without a <converted_image>
    when the <image> is converted to tiff <output_format>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | output_format | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | CROP | x3f_test_files/_SDI8040.X3F.tif | eea588e85f968fe76bb83ca4a5942f7f |
| x3f_test_files/_SDI8040.X3F | UNPROCESSED | x3f_test_files/_SDI8040.X3F.tif | 6ed59ebfad84c57f2b9eb8531c61d17c |
| x3f_test_files/_SDI8040.X3F | COLOR_SRGB | x3f_test_files/_SDI8040.X3F.tif | 86eadcd7f2d7c4fc7043e0a5a05a2887 |
| x3f_test_files/_SDI8040.X3F | COLOR_ADOBE_RGB | x3f_test_files/_SDI8040.X3F.tif | b1d0ccd9b268d6e6112036a56661d60a |
| x3f_test_files/_SDI8040.X3F | COLOR_PROPHOTO_RGB | x3f_test_files/_SDI8040.X3F.tif | 6b360088dd3ac1d9edc008a0be3232a2 |

| x3f_test_files/_SDI8284.X3F | CROP | x3f_test_files/_SDI8284.X3F.tif | e5924b175fe0c0a3ca66b01d12146b42 |
| x3f_test_files/_SDI8284.X3F | UNPROCESSED | x3f_test_files/_SDI8284.X3F.tif | b7215ba5bb0f5a576650512b88f6e199 |
| x3f_test_files/_SDI8284.X3F | QTOP | x3f_test_files/_SDI8284.X3F.tif | 5fce6a9990adc4400adb13f102caca3f |
| x3f_test_files/_SDI8284.X3F | COLOR_SRGB | x3f_test_files/_SDI8284.X3F.tif | 6ebfd835a023512151ba17c34d4dde59 |
| x3f_test_files/_SDI8284.X3F | COLOR_ADOBE_RGB | x3f_test_files/_SDI8284.X3F.tif | d51a8a0e25ac60f1c55b469fd83f24b9 |
| x3f_test_files/_SDI8284.X3F | COLOR_PROPHOTO_RGB | x3f_test_files/_SDI8284.X3F.tif | 558848876ef481e71801dd10d85b1a70 |
