----------------------------------------------------------------
X3F Tools is a library for accessing Sigma X3F raw image files
----------------------------------------------------------------

Reading of the format:

The code understands the old format for SD9, SD10 and SD14. It also
understands the new format introduced with DP1. The latter format is
associated with the TRUE engines, TRUE I and TRUE II. Currently it
also supports the Merrill version and up to the slightly different
Quattro version of the format.

Converting to color images:

The code can convert all the above formats, except the Polaroid x530,
to DNG files. Those DNG files contain color conversion data so that
e.g. Lightroom can convert them to color-correct images.

The code can also convert to color-correct TIFF images, but out of
gamut colors are not handled correctly, in particular if the input
channels are clipped. The latter leads to e.g. colorful skies.

----------------------------------------------------------------
Included in the library are two tools:

  x3f_extract    A tool that extracts JPEG thumbnails and raw images.
                 See below for usage. The RAW images may also be
		 converted to DNG or TIFF color images. Metadata and
		 histograms over the data might also be written.

  x3f_io_test    A tool that prints some metadata and tests that
                 the code is working properly. This tool is not
                 made to be user friendly. It is mainly a testing
                 tool used for development.
----------------------------------------------------------------

----------------------------------------------------------------
Building
----------------------------------------------------------------

You are supposed to have gcc, gmake and cmake installed on your
machine. Currently you need to build on Linux with cross-compilation
for Windows and OSX.

The command "make" builds the executables.

The makefile tries to find out which platform you are on. This might
fail if your system has unexpected properties. In that case you are on
your own and have to hack the makefile.

----------------------------------------------------------------
Usage of the x3f_extract tool
----------------------------------------------------------------

You will get complete information on the switches by running
x3f_extract without any switches, or e.g. with the switch -help.

Here are some examples:

(1) x3f_extract file.x3f
    This one creates the file file.dng for usage in e.g. Lightroom
    or Photoshop (via ACR). The file contains denoised but unconverted
    RAW data plus color conversion info.

(2) x3f_extract -tiff [-color AdobeRGB] file.x3f
    This one creates the file file.tif for usage in e.g. Photoshop.
    The file is fully converted to sRGB, but without rendering intent
    so you might get strange clipping. The optional -color switch shown
    makes it Adobe RGB instead.

(3) x3f_extract -unprocessed file.x3f
    This one creates the file file.tif with raw data. The data is
    linear and unscaled. So, it will generally look black and needs to
    be rescaled and gamma 2.2 has to be applied to it in your editor.

(4) x3f_extract -meta file.x3f
    This one dumps metadata to file.meta

----------------------------------------------------------------
Usage of the x3f_io_test tool
----------------------------------------------------------------

This tool is really only a debugging tool for development. The tool
can be used thus:

(1) x3f_io_test file.x3f
    Reads the file, parses the main data structure, and then prints
    some info about it. NOTE - no parsing of data blocks, e.g. image
    blocks, is done.

(2) x3f_io_test -unpack file.x3f
    Same  as (1), but also prints info from the parsed data blocks.
