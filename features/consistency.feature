Feature: the output produced by the converter is stable

Scenario Outline: conversions to various outputs will produce exactly the same images
   Given an input image <image> without a <converted_image>
    when the <image> is converted by the code to <file_type>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | file_type | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | DNG | x3f_test_files/_SDI8040.X3F.dng | 7ac2cd52a6f45a08bb87eede4b24caa4 |
| x3f_test_files/_SDI8040.X3F | TIFF | x3f_test_files/_SDI8040.X3F.tif | cb67d12ec0a4fd318a426276f527f1a9 |
| x3f_test_files/_SDI8040.X3F | PPM | x3f_test_files/_SDI8040.X3F.ppm | a2bea89af18bb24efd289b73007b2413 |
| x3f_test_files/_SDI8040.X3F | JPG | x3f_test_files/_SDI8040.X3F.jpg | 357f126f6435345642bfbc6171745d00 |
| x3f_test_files/_SDI8284.X3F | DNG | x3f_test_files/_SDI8284.X3F.dng | da01042050d2293b4634bb70404a8e5e |
| x3f_test_files/_SDI8284.X3F | TIFF | x3f_test_files/_SDI8284.X3F.tif | 26199384d894ae723292e5ecc40ad194 |
| x3f_test_files/_SDI8284.X3F | PPM | x3f_test_files/_SDI8284.X3F.ppm | 06c85f54fe3a954d4e564efbbb8d8a8b |
| x3f_test_files/_SDI8284.X3F | JPG | x3f_test_files/_SDI8284.X3F.jpg | 87cd494d3bc4eab4e481de6afeb058de |