/* X3F_EXTRACT.C - Extracting images from X3F files
 * 
 * Copyright 2010 (c) - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 * 
 */

#include "x3f_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {RAW, TIFF, DNG, PPMP3, PPMP6, HISTOGRAM} raw_file_type_t;

static void usage(char *progname)
{
  fprintf(stderr,
          "usage: %s <SWITCHES> <file1> ...\n"
          "   -jpg            Dump embedded JPG. Turn off RAW dumping\n"
          "   -raw            Dump RAW area undecoded\n"
          "   -tiff           Dump RAW as 3x16 bit TIFF (default)\n"
          "   -dng            Dump RAW as DNG LinearRaw\n"
          "   -ppm-ascii      Dump RAW/color as 3x16 bit PPM/P3 (ascii)\n"
          "                   NOTE: 16 bit PPM/P3 is not generally supported\n"
          "   -ppm            Dump RAW/color as 3x16 bit PPM/P6 (binary)\n"
          "   -histogram      Dump histogram as csv file\n"
          "   -loghist        Dump histogram as csv file, with log exposure\n"
	  "   -color <COLOR>  Convert to RGB color\n"
	  "                   (sRGB, AdobeRGB, ProPhotoRGB)\n"
          "   -crop           Crop to active area\n"
          "   -denoise        Denoise RAW data\n"
          "   -wb <WB>        Select white balance preset\n"
	  "\n"
	  "STRANGE STUFF\n"
          "   -offset <OFF>   Offset for SD14 and older\n"
          "                   NOTE: If not given, then offset is automatic\n"
          "   -matrixmax <M>  Max num matrix elements in metadata (def=100)\n",
          progname);
  exit(1);
}

int main(int argc, char *argv[])
{
  int extract_jpg = 0;
  int extract_meta = 0;
  int extract_raw = 1;
  int crop = 0;
  int denoise = 0;
  raw_file_type_t file_type = TIFF;
  x3f_color_encoding_t color_encoding = NONE;
  int files = 0;
  int log_hist = 0;
  char *wb = NULL;

  int i;

  /* Set stdout and stderr to line buffered mode to avoid scrambling */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  for (i=1; i<argc; i++)
    if (!strcmp(argv[i], "-jpg"))
      extract_raw = 0, extract_jpg = 1;
    else if (!strcmp(argv[i], "-meta"))
      extract_raw = 0, extract_meta = 1;
    else if (!strcmp(argv[i], "-raw"))
      extract_raw = 1, file_type = RAW;
    else if (!strcmp(argv[i], "-tiff"))
      extract_raw = 1, file_type = TIFF;
    else if (!strcmp(argv[i], "-dng"))
      extract_raw = 1, file_type = DNG;
    else if (!strcmp(argv[i], "-ppm-ascii"))
      extract_raw = 1, file_type = PPMP3;
    else if (!strcmp(argv[i], "-ppm"))
      extract_raw = 1, file_type = PPMP6;
    else if (!strcmp(argv[i], "-histogram"))
      extract_raw = 1, file_type = HISTOGRAM;
    else if (!strcmp(argv[i], "-loghist"))
      extract_raw = 1, file_type = HISTOGRAM, log_hist = 1;
    else if (!strcmp(argv[i], "-color") && (i+1)<argc) {
      char *encoding = argv[++i];
      if (!strcmp(encoding, "sRGB"))
	color_encoding = SRGB;
      else if (!strcmp(encoding, "AdobeRGB"))
	color_encoding = ARGB;
      else if (!strcmp(encoding, "ProPhotoRGB"))
	color_encoding = PPRGB;
      else {
	fprintf(stderr, "Unknown color encoding: %s\n", encoding);
	usage(argv[0]);
      }
    }
    else if (!strcmp(argv[i], "-crop"))
      crop = 1;
    else if (!strcmp(argv[i], "-denoise"))
      denoise = 1;
    else if ((!strcmp(argv[i], "-wb")) && (i+1)<argc)
      wb = argv[++i];

  /* Strange Stuff */
    else if ((!strcmp(argv[i], "-offset")) && (i+1)<argc)
      legacy_offset = atoi(argv[++i]), auto_legacy_offset = 0;
    else if ((!strcmp(argv[i], "-matrixmax")) && (i+1)<argc)
      max_printed_matrix_elements = atoi(argv[++i]);
    else if (!strncmp(argv[i], "-", 1))
      usage(argv[0]);
    else
      break;			/* Here starts list of files */

  for (; i<argc; i++) {
    char *infilename = argv[i];
    FILE *f_in = fopen(infilename, "rb");
    x3f_t *x3f = NULL;

    files++;

    if (f_in == NULL) {
      fprintf(stderr, "Could not open infile %s\n", infilename);
      goto clean_up;
    }

    printf("READ THE X3F FILE %s\n", infilename);
    x3f = x3f_new_from_file(f_in);

    if (x3f == NULL) {
      fprintf(stderr, "Could not read infile %s\n", infilename);
      goto clean_up;
    }

    if (extract_jpg) {
      char outfilename[1024];
      x3f_return_t ret;

      x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f));

      strcpy(outfilename, infilename);
      strcat(outfilename, ".jpg");

      printf("Dump JPEG to %s\n", outfilename);
      if (X3F_OK != (ret=x3f_dump_jpeg(x3f, outfilename)))
        fprintf(stderr, "Could not dump JPEG to %s: %s\n",
		outfilename, x3f_err(ret));
    }

    if (extract_meta || extract_raw) {
      /* We assume we do not need JPEG meta data
	 x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f)); */
      x3f_load_data(x3f, x3f_get_prop(x3f));
      x3f_load_data(x3f, x3f_get_camf(x3f));
    }

    if (extract_meta) {
      char outfilename[1024];
      x3f_return_t ret;

      strcpy(outfilename, infilename);
      strcat(outfilename, ".meta");

      printf("Dump META DATA to %s\n", outfilename);
      if (X3F_OK != (ret=x3f_dump_meta_data(x3f, outfilename)))
        fprintf(stderr, "Could not dump META DATA to %s: %s\n",
		outfilename, x3f_err(ret));
    }

    if (extract_raw) {
      char outfilename[1024];
      x3f_return_t ret_dump = X3F_OK;

      printf("Load RAW block from %s\n", infilename);
      if (file_type == RAW) {
        if (X3F_OK != x3f_load_image_block(x3f, x3f_get_raw(x3f))) {
          fprintf(stderr, "Could not load unconverted RAW from memory\n");
          goto clean_up;
        }
      } else {
        if (X3F_OK != x3f_load_data(x3f, x3f_get_raw(x3f))) {
          fprintf(stderr, "Could not load RAW from memory\n");
          goto clean_up;
        }
      }

      strcpy(outfilename, infilename);

      switch (file_type) {
      case RAW:
	strcat(outfilename, ".raw");
	printf("Dump RAW block to %s\n", outfilename);
	ret_dump = x3f_dump_raw_data(x3f, outfilename);
	break;
      case TIFF:
	strcat(outfilename, ".tif");
	printf("Dump RAW as TIFF to %s\n", outfilename);
	ret_dump = x3f_dump_raw_data_as_tiff(x3f, outfilename,
					     color_encoding, crop, denoise, wb);
	break;
      case DNG:
	strcat(outfilename, ".dng");
	printf("Dump RAW as DNG to %s\n", outfilename);
	ret_dump = x3f_dump_raw_data_as_dng(x3f, outfilename, denoise, wb);
	break;
      case PPMP3:
      case PPMP6:
	strcat(outfilename, ".ppm");
	printf("Dump RAW as PPM to %s\n", outfilename);
	ret_dump = x3f_dump_raw_data_as_ppm(x3f, outfilename,
					    color_encoding, crop, denoise, wb,
                                            file_type == PPMP6);
	break;
      case HISTOGRAM:
	strcat(outfilename, ".csv");
	printf("Dump RAW as CSV histogram to %s\n", outfilename);
	ret_dump = x3f_dump_raw_data_as_histogram(x3f, outfilename,
						  color_encoding,
						  crop, denoise, wb,
						  log_hist);
	break;
      }

      if (X3F_OK != ret_dump)
        fprintf(stderr, "Could not dump RAW to %s: %s\n",
		outfilename, x3f_err(ret_dump));
    }

  clean_up:

    x3f_delete(x3f);

    if (f_in != NULL)
      fclose(f_in);
  }

  if (files == 0)
    usage(argv[0]);

  return 0;
}
