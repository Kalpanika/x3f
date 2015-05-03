Feature: the output produced by the converter is stable

Scenario Outline: conversions to various outputs will produce exactly the same images
   Given an input image <image>
    when the <image> is converted by the code to <file_type>
    then the <converted_image> has the right <md5> hash value

Examples: images
| image | file_type | converted_image | md5 |
| x3f_test_files/_SDI8040.X3F | DNG | x3f_test_files/_SDI8040.dng | hash-value |
| x3f_test_files/_SDI8040.X3F | TIFF | x3f_test_files/_SDI8040.tiff | hash-value |
| x3f_test_files/_SDI8284.X3F | DNG | x3f_test_files/_SDI8040.dng | hash-value |
| x3f_test_files/_SDI8284.X3F | TIFF | x3f_test_files/_SDI8040.tiff | hash-value |