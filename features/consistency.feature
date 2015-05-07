Feature: the output produced by the converter is stable


Scenario Outline: conversions to various outputs will produce exactly the same images
   Given an input image <image> without a <converted_image>
    when the <image> is converted by the code to <file_type>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | file_type | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | DNG | x3f_test_files/_SDI8040.X3F.dng | 5a31b26ab81a2d1d12107f1fd8bc9c12 |
| x3f_test_files/_SDI8040.X3F | TIFF | x3f_test_files/_SDI8040.X3F.tif | cb67d12ec0a4fd318a426276f527f1a9 |
| x3f_test_files/_SDI8040.X3F | PPM | x3f_test_files/_SDI8040.X3F.ppm | a2bea89af18bb24efd289b73007b2413 |
| x3f_test_files/_SDI8040.X3F | JPG | x3f_test_files/_SDI8040.X3F.jpg | 357f126f6435345642bfbc6171745d00 |
| x3f_test_files/_SDI8040.X3F | META | x3f_test_files/_SDI8040.X3F.meta | 6a29dd5d1284e453bcf6807f230d7180 |
| x3f_test_files/_SDI8040.X3F | RAW | x3f_test_files/_SDI8040.X3F.raw | 89582789c24f86aaefd92395bc4f0572 |
| x3f_test_files/_SDI8040.X3F | PPM-ASCII | x3f_test_files/_SDI8040.X3F.ppm | 74e6652a6a1325949d086bc9f099fbe7 |
| x3f_test_files/_SDI8040.X3F | HISTOGRAM | x3f_test_files/_SDI8040.X3F.csv | 8ad3868b2c7b871555932a783c16397d |
| x3f_test_files/_SDI8040.X3F | LOGHIST | x3f_test_files/_SDI8040.X3F.csv | 0c23a7767659c894b81ba64326517eba |

| x3f_test_files/_SDI8284.X3F | DNG | x3f_test_files/_SDI8284.X3F.dng | e8bfc6373dd66753b540f3b604f9d657 |
| x3f_test_files/_SDI8284.X3F | TIFF | x3f_test_files/_SDI8284.X3F.tif | 26199384d894ae723292e5ecc40ad194 |
| x3f_test_files/_SDI8284.X3F | PPM | x3f_test_files/_SDI8284.X3F.ppm | 06c85f54fe3a954d4e564efbbb8d8a8b |
| x3f_test_files/_SDI8284.X3F | JPG | x3f_test_files/_SDI8284.X3F.jpg | 87cd494d3bc4eab4e481de6afeb058de |
| x3f_test_files/_SDI8284.X3F | META | x3f_test_files/_SDI8284.X3F.meta | d93655234be234bdb6c49183bc683eea |
| x3f_test_files/_SDI8284.X3F | RAW | x3f_test_files/_SDI8284.X3F.raw | b9d230401b9822978b49ec244c2f0668 |
| x3f_test_files/_SDI8284.X3F | PPM-ASCII | x3f_test_files/_SDI8284.X3F.ppm | cf48c36eddbddccefcf44378d2e45ba8 |
| x3f_test_files/_SDI8284.X3F | HISTOGRAM | x3f_test_files/_SDI8284.X3F.csv | 78edd0177ac057d77ed62d560267b640 |
| x3f_test_files/_SDI8284.X3F | LOGHIST | x3f_test_files/_SDI8284.X3F.csv | 9a8f38c290c52858200166282daba772 |


Scenario Outline: denoised conversions to dng will produce the exact same outputs
   Given an input image <image> without a <converted_image>
    when the <image> is denoised and converted by the code
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | x3f_test_files/_SDI8040.X3F.dng | 06ab38041552b592340bebfad7fc52ba |
| x3f_test_files/_SDI8284.X3F | x3f_test_files/_SDI8284.X3F.dng | 5deed4f972edf2bbd1fde5024a5f21f8 |


Scenario Outline: denoised conversions to tiff will produce the exact same outputs
   Given an input image <image> without a <converted_image>
    when the <image> is denoised and converted by the code to a cropped color TIFF
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | x3f_test_files/_SDI8040.X3F.tif | c15d8761cbcaffd2ab381b9549a31e6b |
| x3f_test_files/_SDI8284.X3F | x3f_test_files/_SDI8284.X3F.tif | 8411d4327bf6e71be8f4f8161b98a6b4 |


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
| x3f_test_files/_SDI8040.X3F | COLOR_PROPHOTO_RGB | x3f_test_files/_SDI8040.X3F.tif | 719320a87c58c47d9a7879e483e6845d |

| x3f_test_files/_SDI8284.X3F | CROP | x3f_test_files/_SDI8284.X3F.tif | e5924b175fe0c0a3ca66b01d12146b42 |
| x3f_test_files/_SDI8284.X3F | UNPROCESSED | x3f_test_files/_SDI8284.X3F.tif | b7215ba5bb0f5a576650512b88f6e199 |
| x3f_test_files/_SDI8284.X3F | QTOP | x3f_test_files/_SDI8284.X3F.tif | 5fce6a9990adc4400adb13f102caca3f |
| x3f_test_files/_SDI8284.X3F | COLOR_SRGB | x3f_test_files/_SDI8284.X3F.tif | 494b747b2ad9489977a44e830fcc7d18 |
| x3f_test_files/_SDI8284.X3F | COLOR_ADOBE_RGB | x3f_test_files/_SDI8284.X3F.tif | c9cfc3a694f24f909b4b5c99759996c8 |
| x3f_test_files/_SDI8284.X3F | COLOR_PROPHOTO_RGB | x3f_test_files/_SDI8284.X3F.tif | dbc7db4d8e8602a3da4aada07c079b1c |
