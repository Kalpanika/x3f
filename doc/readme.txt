----------------------------------------------------------------
X3F tools is a library for accessing Sigma X3F raw image files
----------------------------------------------------------------

Reading of format:

The code understands  the old format for SD9, SD10  and SD14. The code
also understands the new format introduced with DP1. The latter format
is associated with the TRUE engines  TRUE I and TRUE II. Currently the
code  supports  also  the  Merrill  version and  up  to  the  slightly
different Quattro version of the cameras.

Converting to color images:

The code can convert all above formats, except the Polaroid X370, to a
DNG  file.  This DNG  file  contains  color  conversion date  so  that
e.g. Lightroom can convert to correct color images.

The code can also convert to color TIFF images, but out of gamut
colors are not handled correctly. In particular if the input channels
are clipped. The latter leads to e.g. colorful skies.

----------------------------------------------------------------
Included in the library are two tools

  x3f_extract    A tool that extracts JPEG thumbnail and raw images.
                 See below for usage. The RAW images may also be
		 converted to DNG or TIFF color images. Meta data and
		 histograms over the data might also be written.

  x3f_io_test    A tool that prints some meta data and tests that
                 the code is working properly. This tool is not
                 made to be user friendly. Its mainly a test
                 tool for developing the code.
----------------------------------------------------------------

----------------------------------------------------------------
Executables
----------------------------------------------------------------

It  is a  fair  chance that  you  can find  pre  built executables  on
http://www.proxel.se/x3f.html.

----------------------------------------------------------------
Building
----------------------------------------------------------------

You are  supposed to have installed  gcc (and gmake) on  your machine.
Currently  you need  to  build  on Linux  with  cross compilation  for
Windows and OSX.

The command "make" builds the executables.

The makefile  tries to find out  what platform you are  on. This might
fail if your system has unexpected properties. In this case you are on
your own and have to hack the makefile file.

----------------------------------------------------------------
Usage of the x3f_extract tool
----------------------------------------------------------------

You will get a total information on switches by running x3f_extract
without any switches, or e.g. with the switch -help.

Here are some examples:

(1) x3f_extract -denoise file.x3f
    This one creates the file file.dng for usage in e.g. Lightroom
    or Photoshop (via ACR). The file contains denoised but unconverted
    RAW data plus color conversion info.

(2) x3f_extract -denoise -tiff -color sRGB file.x3f
    This one creates the file file.tiff for usage in e.g. Photoshop.
    The file is fully converted to sRGB, but without rendering intent
    so you might get strange clipping.

(3) x3f_extract -unprocessed file.x3f
    This one creates the file file.tiff with RAW data. This data is linear
    and unscaled. So, it is generally look black and needs to be scaled
    and applied gamma 2.2 in your editor.

(4) x3f_extract -meta file.x3f
    This one dumnps meta data in file.meta


----------------------------------------------------------------
Usage of the x3f_io_test tool
----------------------------------------------------------------

This tool is really only a developer debug tool.
The tool can be used thus:

(1) x3f_io_test file.x3f
    Reads the file and parses  the main data structure of the file and
    then prints some info about it.  NOTE - no parsing of data blocks,
    e.g. image blocks, is made.

(2) x3f_io_test -unpack file.x3f
    Same  as (1), but also writes from the parsed version.
