/* X3F_EXTRACT.C - Extracting images from X3F files
 *
 * Copyright 2010-2015 (c) - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_io.h"
#include "x3f_process.h"
#include "x3f_output_dng.h"
#include "x3f_output_tiff.h"
#include "x3f_output_ppm.h"
#include "x3f_histogram.h"
#include "x3f_print.h"
#include "x3f_dump.h"
#include "x3f_denoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
  { META      = 0,
    JPEG      = 1,
    RAW       = 2,
    TIFF      = 3,
    DNG       = 4,
    PPMP3     = 5,
    PPMP6     = 6,
    HISTOGRAM = 7}
  output_file_type_t;

static char *extension[] =
  { ".meta",
    ".jpg",
    ".raw",
    ".tif",
    ".dng",
    ".ppm",
    ".ppm",
    ".csv" };

static void usage(char *progname)
{
  fprintf(stderr,
          "usage: %s <SWITCHES> <file1> ...\n"
          "   -o <DIR>        Use <DIR> as output dir\n"
	  "ONE OFF THE FORMAT SWITCHWES\n"
	  "   -meta           Dump meta data\n"
          "   -jpg            Dump embedded JPG. Turn off RAW dumping\n"
          "   -raw            Dump RAW area undecoded\n"
          "   -tiff           Dump RAW as 3x16 bit TIFF\n"
          "   -dng            Dump RAW as DNG LinearRaw (default)\n"
          "   -ppm-ascii      Dump RAW/color as 3x16 bit PPM/P3 (ascii)\n"
          "                   NOTE: 16 bit PPM/P3 is not generally supported\n"
          "   -ppm            Dump RAW/color as 3x16 bit PPM/P6 (binary)\n"
          "   -histogram      Dump histogram as csv file\n"
          "   -loghist        Dump histogram as csv file, with log exposure\n"
	  "APPROPRIATE COMBINATIONS OF MODIFIER SWITCHES\n"
	  "   -color <COLOR>  Convert to RGB color\n"
	  "                   (sRGB, AdobeRGB, ProPhotoRGB)\n"
          "   -unprocessed    Dump RAW without any preprocessing\n"
          "   -qtop           Dump Quattro top layer without preprocessing\n"
          "   -crop           Crop to active area\n"
          "   -denoise        Denoise RAW data\n"
          "   -wb <WB>        Select white balance preset\n"
          "   -ocl            Use OpenCL\n"
	  "\n"
	  "STRANGE STUFF\n"
          "   -offset <OFF>   Offset for SD14 and older\n"
          "                   NOTE: If not given, then offset is automatic\n"
          "   -matrixmax <M>  Max num matrix elements in metadata (def=100)\n",
          progname);
  exit(1);
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static int check_dir(char *Path)
{
  struct stat filestat;
  int ret;

  if ((ret = stat(Path, &filestat)) != 0)
    return ret;

  if (!S_ISDIR(filestat.st_mode)) {
    errno = ENOTDIR;
    return -1;
  }

  return 0;
}

#define MAXPATH 1000
#define EXTMAX 10
#define MAXOUTPATH (MAXPATH+EXTMAX)
#define MAXTMPPATH (MAXOUTPATH+EXTMAX)

static int safecpy(char *dst, const char *src, int dst_size)
{
  if (strnlen(src, dst_size+1) > dst_size) {
    fprintf(stderr, "safecpy: String too large for dst name\n");
    return 1;
  } else {
    strcpy(dst, src);
    return 0;
  }
}

static int safecat(char *dst, const char *src, int dst_size)
{
  if (strnlen(dst, dst_size+1) + strnlen(src, dst_size+1) > dst_size) {
    fprintf(stderr, "safecat: String too large for dst name\n");
    return 1;
  } else {
    strcat(dst, src);
    return 0;
  }
}

static int make_paths(const char *inpath, const char *outdir,
		      const char *ext,
		      char *tmppath, char *outpath)
{
  int err = 0;
  const char *plainoutpath;
  char pathbuffer[MAXPATH+1];

  if (outdir == NULL) {
    plainoutpath = inpath;
  } else {
    char *ptr = strrchr(inpath, '/');

    err += safecpy(pathbuffer, outdir, MAXPATH);
    err += safecat(pathbuffer, "/", MAXPATH);
    if (ptr == NULL) {
      err += safecat(pathbuffer, inpath, MAXPATH);
    } else {
      err += safecat(pathbuffer, ptr+1, MAXPATH);
    }

    plainoutpath = pathbuffer;
  }

  err += safecpy(outpath, plainoutpath, MAXOUTPATH);
  err += safecat(outpath, ext, MAXOUTPATH);
  err += safecpy(tmppath, outpath, MAXTMPPATH);
  err += safecat(tmppath, ".tmp", MAXTMPPATH);

  return err;
}

#define Z extract_jpg=0,extract_raw=0,extract_unconverted_raw=0

int main(int argc, char *argv[])
{
  int extract_jpg = 0;
  int extract_meta; /* Always computed */
  int extract_raw = 1;
  int extract_unconverted_raw = 1;
  int crop = 0;
  int denoise = 0;
  output_file_type_t file_type = DNG;
  x3f_color_encoding_t color_encoding = NONE;
  int files = 0;
  int errors = 0;
  int log_hist = 0;
  char *wb = NULL;
  int use_opencl = 0;
  char *outdir = NULL;

  int i;

  /* Set stdout and stderr to line buffered mode to avoid scrambling */
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  for (i=1; i<argc; i++)

    /* Only one of those switches is valid, the last one */
    if (!strcmp(argv[i], "-jpg"))
      Z, extract_jpg = 1, file_type = JPEG;
    else if (!strcmp(argv[i], "-meta"))
      Z, file_type = META;
    else if (!strcmp(argv[i], "-raw"))
      Z, extract_unconverted_raw = 1, file_type = RAW;
    else if (!strcmp(argv[i], "-tiff"))
      Z, extract_raw = 1, file_type = TIFF;
    else if (!strcmp(argv[i], "-dng"))
      Z, extract_raw = 1, file_type = DNG;
    else if (!strcmp(argv[i], "-ppm-ascii"))
      Z, extract_raw = 1, file_type = PPMP3;
    else if (!strcmp(argv[i], "-ppm"))
      Z, extract_raw = 1, file_type = PPMP6;
    else if (!strcmp(argv[i], "-histogram"))
      Z, extract_raw = 1, file_type = HISTOGRAM;
    else if (!strcmp(argv[i], "-loghist"))
      Z, extract_raw = 1, file_type = HISTOGRAM, log_hist = 1;

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
    else if (!strcmp(argv[i], "-o") && (i+1)<argc) {
      outdir = argv[++i];
    }
    else if (!strcmp(argv[i], "-unprocessed"))
      color_encoding = UNPROCESSED;
    else if (!strcmp(argv[i], "-qtop"))
      color_encoding = QTOP;
    else if (!strcmp(argv[i], "-crop"))
      crop = 1;
    else if (!strcmp(argv[i], "-denoise"))
      denoise = 1;
    else if ((!strcmp(argv[i], "-wb")) && (i+1)<argc)
      wb = argv[++i];
    else if (!strcmp(argv[i], "-ocl"))
      use_opencl = 1;

  /* Strange Stuff */
    else if ((!strcmp(argv[i], "-offset")) && (i+1)<argc)
      legacy_offset = atoi(argv[++i]), auto_legacy_offset = 0;
    else if ((!strcmp(argv[i], "-matrixmax")) && (i+1)<argc)
      max_printed_matrix_elements = atoi(argv[++i]);
    else if (!strncmp(argv[i], "-", 1))
      usage(argv[0]);
    else
      break;			/* Here starts list of files */

  if (outdir != NULL && check_dir(outdir) != 0) {
    fprintf(stderr, "Could not find outdir %s\n", outdir);
    usage(argv[0]);
  }

  x3f_set_use_opencl(use_opencl);

  extract_meta =
    file_type == META ||
    file_type == DNG ||
    (extract_raw &&
     (crop || (color_encoding != UNPROCESSED && color_encoding != QTOP)));

  for (; i<argc; i++) {
    char *infile = argv[i];
    FILE *f_in = fopen(infile, "rb");
    x3f_t *x3f = NULL;

    char tmpfile[MAXTMPPATH+1];
    char outfile[MAXOUTPATH+1];
    x3f_return_t ret_dump;

    files++;

    if (f_in == NULL) {
      fprintf(stderr, "Could not open infile %s\n", infile);
      goto found_error;
    }

    printf("READ THE X3F FILE %s\n", infile);
    x3f = x3f_new_from_file(f_in);

    if (x3f == NULL) {
      fprintf(stderr, "Could not read infile %s\n", infile);
      goto found_error;
    }

    if (extract_jpg) {
      if (X3F_OK != x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f))) {
	fprintf(stderr, "Could not load JPEG thumbnail from file\n");
	goto found_error;
      }
    }

    if (extract_meta) {
      x3f_directory_entry_t *DE = x3f_get_prop(x3f);

      if (X3F_OK != x3f_load_data(x3f, x3f_get_camf(x3f))) {
	fprintf(stderr, "Could not load CAMF from file\n");
	goto found_error;
      }
      if (DE != NULL)
	/* Not for Quattro */
	if (X3F_OK != x3f_load_data(x3f, DE)) {
	  fprintf(stderr, "Could not load PROP from file\n");
	  goto found_error;
	}
      /* We do not load any JPEG meta data */
    }

    if (extract_raw) {
      if (X3F_OK != x3f_load_data(x3f, x3f_get_raw(x3f))) {
	fprintf(stderr, "Could not load RAW from file\n");
	goto found_error;
      }
    }

    if (extract_unconverted_raw) {
      if (X3F_OK != x3f_load_image_block(x3f, x3f_get_raw(x3f))) {
	fprintf(stderr, "Could not load unconverted RAW from file\n");
	goto found_error;
      }
    }

    if (make_paths(infile, outdir, extension[file_type], tmpfile, outfile)) {
      fprintf(stderr, "Too large file path\n");
      goto found_error;
    }

    switch (file_type) {
    case META:
      printf("Dump META DATA to %s\n", outfile);
      ret_dump = x3f_dump_meta_data(x3f, tmpfile);
      break;
    case JPEG:
      printf("Dump JPEG to %s\n", outfile);
      ret_dump = x3f_dump_jpeg(x3f, tmpfile);
      break;
    case RAW:
      printf("Dump RAW block to %s\n", outfile);
      ret_dump = x3f_dump_raw_data(x3f, tmpfile);
      break;
    case TIFF:
      printf("Dump RAW as TIFF to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_tiff(x3f, tmpfile,
					   color_encoding, crop, denoise, wb);
      break;
    case DNG:
      printf("Dump RAW as DNG to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_dng(x3f, tmpfile, denoise, wb);
      break;
    case PPMP3:
    case PPMP6:
      printf("Dump RAW as PPM to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_ppm(x3f, tmpfile,
					  color_encoding, crop, denoise, wb,
					  file_type == PPMP6);
      break;
    case HISTOGRAM:
      printf("Dump RAW as CSV histogram to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_histogram(x3f, tmpfile,
						color_encoding,
						crop, denoise, wb,
						log_hist);
      break;
    }

    if (X3F_OK != ret_dump) {
      fprintf(stderr, "Could not dump to %s: %s\n",
	      tmpfile, x3f_err(ret_dump));
      errors++;
    } else {
      if (rename(tmpfile, outfile) != 0) {
	fprintf(stderr, "Couldn't rename %s to %s\n", tmpfile, outfile);
	errors++;
      }
    }

    goto clean_up;

  found_error:

    errors++;

  clean_up:

    x3f_delete(x3f);

    if (f_in != NULL)
      fclose(f_in);
  }

  if (files == 0) {
    fprintf(stderr, "No files given\n");
    usage(argv[0]);
  }

  printf("Files processed: %d\terrors: %d\n", files, errors);

  return errors > 0;
}
