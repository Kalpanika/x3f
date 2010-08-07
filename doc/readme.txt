----------------------------------------------------------------
X3F tools is a library for accessing Sigma X3F raw image files
----------------------------------------------------------------

The code understands  the old format for SD9, SD10  and SD14. The code
also understands the new format introduced with DP1. The latter format
is associated with the TRUE engines TRUE I and TRUE II.

----------------------------------------------------------------
Included in the library are two tools
  x3f_extract    A tool that extracts JPEG thumbnail and raw images.
                 See below for usage.
  x3f_io_test    A tool that prints some meta data and tests that
                 the code is working properly. This tool is not
                 made to be user friendly. Its mainly a test
                 tool for developing the code. This tool can
                 also write x3f files. They should (with this
                 tool) be identical to the origanal file.
                 See below for usage.
----------------------------------------------------------------

----------------------------------------------------------------
Executables
----------------------------------------------------------------

It  is a  fair  chance that  you  can find  pre  built executables  on
http://www.proxel.se/x3f.html.

----------------------------------------------------------------
Building
----------------------------------------------------------------

You are  supposed to have installed  gcc (and gmake)  on your machine.
On Windows this currently (2010) means mingw.

The command "make" builds the executables.

The makefile  tries to find out  what platform you are  on. This might
fail if your system has unexpected properties. In this case you are on
your own and have to hack the makefile file.

----------------------------------------------------------------
Usage of the x3f_extract tool
----------------------------------------------------------------

The tool can be used thus:

(1) x3f_extract -raw file.x3f
(2) x3f_extract -tiff file.x3f
(3) x3f_extract -tiff -gamma 2.2 [-min 0] [-max 5000] file.x3f
(4) x3f_extract -jpeg file.x3f

(1) dumps the  data block of the RAW image  verbatim. The original RAW
    data is not parsed or  interpreted and is therefore for almost all
    aspects useless. Mainly used for analyzing a new unknown format.

(2) parses and interprets the RAW image data and then dumps the result
    as  a 16  bit TIFF  file (called  file.x3f.tif) without  doing any
    changes to the pixel values.

    NOTE - if  you load this TIFF file in e.g.  Photoshop - the result
    is a very dark (almost  black) image. This image needs scaling and
    gamma coding to look good.

    NOTE -  the result is  not a  regular RGB image  - its in  the X3F
    "color space" - so it contains faulty colors.

(3) is the same as (2) but it scales the image according to
    a min, a max and a gamma value. If the min and max values are not
    given then they are estimated.

    NOTE  - the  current  code does  not  really know  how to  compute
    the correct min and max values. Its recommended to set it yourself
    - a guess is that max might be between 5000 and 10000. The program
    writes the min and max value it finds. If you are outside the range,
    the value is clipped.

(4) dumps the embedded JPEG thumbnail verbatim as file.x3f.jpg

    NOTE - this is not a JPEG of the RAW data.


----------------------------------------------------------------
Usage of the x3f_io_test tool
----------------------------------------------------------------

The tool can be used thus

(1) x3f_io_test file.x3f
(2) x3f_io_test file.x3f outfile1.x3f
(3) x3f_io_test file.x3f outfile1.x3f outfile2.x3f

(1) reads the file and parses  the main data structure of the file and
    then prints some info about it.  NOTE - no parsing of data blocks,
    e.g. image blocks, is made.

(2) same  as (1) but  also writes what  it reads to the  outfile.  The
    outfile1 shall be identical if the code is correct.

(3) Same  as (2) but  this code also  parses the data blocks  and then
    assembles a new file from the  parsed data blocks and writes it to
    oufile2.  The printouts  are written  twice -  before  parsing the
    blocks  and after.  The outfile2  also shall  be identical  to the
    original file if the code is correct.

    NOTE - detaled assembling of the blocks is not yet implemented, so
    the  assembling  is partly  phony.  Therefore  -  that oufile2  is
    identical does not say so much as it should.

    NOTE  - the printouts  contain CAMF  parameter names.  The current
    implementation stops  parsing CAMF when finding  something it does
    not understand. That might  be wrong. Therefore - parameters might
    be missing.

    NOTE - the actual contents of the CAMF parameters is not yet parsed.
