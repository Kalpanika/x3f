/* X3F_EXTRACT.C
 *
 * Extracting images from X3F files.
 * Also converting to histogram and color image.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_version.h"
#include "x3f_io.h"
#include "x3f_process.h"
#include "x3f_output_dng.h"
#include "x3f_output_tiff.h"
#include "x3f_output_ppm.h"
#include "x3f_histogram.h"
#include "x3f_print_meta.h"
#include "x3f_dump.h"
#include "x3f_denoise.h"
#include "x3f_printf.h"

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
          "   -o <DIR>        Use <DIR> as output directory\n"
          "   -v              Verbose output for debugging\n"
          "   -q              Suppress all messages except errors\n"
	  "ONE OFF THE FORMAT SWITCHWES\n"
	  "   -meta           Dump metadata\n"
          "   -jpg            Dump embedded JPEG\n"
          "   -raw            Dump RAW area undecoded\n"
          "   -tiff           Dump RAW/color as 3x16 bit TIFF\n"
          "   -dng            Dump RAW as DNG LinearRaw (default)\n"
          "   -ppm-ascii      Dump RAW/color as 3x16 bit PPM/P3 (ascii)\n"
          "                   NOTE: 16 bit PPM/P3 is not generally supported\n"
          "   -ppm            Dump RAW/color as 3x16 bit PPM/P6 (binary)\n"
          "   -histogram      Dump histogram as csv file\n"
          "   -loghist        Dump histogram as csv file, with log exposure\n"
	  "APPROPRIATE COMBINATIONS OF MODIFIER SWITCHES\n"
	  "   -color <COLOR>  Convert to RGB color space\n"
	  "                   (none, sRGB, AdobeRGB, ProPhotoRGB)\n"
	  "                   'none' means neither scaling, applying gamma\n"
	  "                   nor converting color space.\n"
	  "                   This switch does not affect DNG output\n"
          "   -unprocessed    Dump RAW without any preprocessing\n"
          "   -qtop           Dump Quattro top layer without preprocessing\n"
          "   -no-crop        Do not crop to active area\n"
          "   -no-denoise     Do not denoise RAW data\n"
          "   -no-sgain       Do not apply spatial gain (color compensation)\n"
          "   -no-fix-bad     Do not fix bad pixels\n"
          "   -sgain          Apply spatial gain (default except for Quattro)\n"
          "   -wb <WB>        Select white balance preset\n"
          "   -compress       Enable ZIP compression for DNG and TIFF output\n"
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
    x3f_printf(DEBUG, "safecpy: String too large\n");
    return 1;
  } else {
    strcpy(dst, src);
    return 0;
  }
}

static int safecat(char *dst, const char *src, int dst_size)
{
  if (strnlen(dst, dst_size+1) + strnlen(src, dst_size+1) > dst_size) {
    x3f_printf(DEBUG, "safecat: String too large\n");
    return 1;
  } else {
    strcat(dst, src);
    return 0;
  }
}

#if defined(_WIN32) || defined(_WIN64)
#define PATHSEP "\\"
static const char pathseps[] = PATHSEP "/:";
#else
#define PATHSEP "/"
static const char pathseps[] = PATHSEP;
#endif

static int make_paths(const char *inpath, const char *outdir,
		      const char *ext,
		      char *tmppath, char *outpath)
{
  int err = 0;

  if (outdir && *outdir) {
    const char *ptr = inpath, *sep, *p;

    for (sep=pathseps; *sep; sep++)
      if ((p = strrchr(inpath, *sep)) && p+1 > ptr) ptr = p+1;

    err += safecpy(outpath, outdir, MAXOUTPATH);
    if (!strchr(pathseps, outdir[strlen(outdir)-1]))
      err += safecat(outpath, PATHSEP, MAXOUTPATH);
    err += safecat(outpath, ptr, MAXOUTPATH);
  }
  else err += safecpy(outpath, inpath, MAXOUTPATH);

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
  int extract_unconverted_raw = 0;
  int crop = 1;
  int fix_bad = 1;
  int denoise = 1;
  int apply_sgain = -1;
  output_file_type_t file_type = DNG;
  x3f_color_encoding_t color_encoding = SRGB;
  int files = 0;
  int errors = 0;
  int log_hist = 0;
  char *wb = NULL;
  int compress = 0;
  int use_opencl = 0;
  char *outdir = NULL;
  x3f_return_t ret;

  int i;

  x3f_printf(INFO, "X3F TOOLS VERSION = %s\n\n", version);

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
      if (!strcmp(encoding, "none"))
	color_encoding = NONE;
      else if (!strcmp(encoding, "sRGB"))
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
    else if (!strcmp(argv[i], "-o") && (i+1)<argc)
      outdir = argv[++i];
    else if (!strcmp(argv[i], "-v"))
      x3f_printf_level = DEBUG;
    else if (!strcmp(argv[i], "-q"))
      x3f_printf_level = ERR;
    else if (!strcmp(argv[i], "-unprocessed"))
      color_encoding = UNPROCESSED;
    else if (!strcmp(argv[i], "-qtop"))
      color_encoding = QTOP;
    else if (!strcmp(argv[i], "-no-crop"))
      crop = 0;
    else if (!strcmp(argv[i], "-no-fix-bad"))
      fix_bad = 0;
    else if (!strcmp(argv[i], "-no-denoise"))
      denoise = 0;
    else if (!strcmp(argv[i], "-no-sgain"))
      apply_sgain = 0;
    else if (!strcmp(argv[i], "-sgain"))
      apply_sgain = 1;
    else if ((!strcmp(argv[i], "-wb")) && (i+1)<argc)
      wb = argv[++i];
    else if (!strcmp(argv[i], "-compress"))
      compress = 1;
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
    x3f_printf(ERR, "Could not find outdir %s\n", outdir);
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
    int sgain;

    files++;

    if (f_in == NULL) {
      x3f_printf(ERR, "Could not open infile %s\n", infile);
      goto found_error;
    }

    x3f_printf(INFO, "READ THE X3F FILE %s\n", infile);
    x3f = x3f_new_from_file(f_in);

    if (x3f == NULL) {
      x3f_printf(ERR, "Could not read infile %s\n", infile);
      goto found_error;
    }

    if (extract_jpg) {
      if (X3F_OK != (ret = x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f)))) {
	x3f_printf(ERR, "Could not load JPEG thumbnail from %s (%s)\n",
		   infile, x3f_err(ret));
	goto found_error;
      }
    }

    if (extract_meta) {
      x3f_directory_entry_t *DE = x3f_get_prop(x3f);

      if (X3F_OK != (ret = x3f_load_data(x3f, x3f_get_camf(x3f)))) {
	x3f_printf(ERR, "Could not load CAMF from %s (%s)\n",
		   infile, x3f_err(ret));
	goto found_error;
      }
      if (DE != NULL)
	/* Not for Quattro */
	if (X3F_OK != (ret = x3f_load_data(x3f, DE))) {
	  x3f_printf(ERR, "Could not load PROP from %s (%s)\n",
		     infile, x3f_err(ret));
	  goto found_error;
	}
      /* We do not load any JPEG meta data */
    }

    if (extract_raw) {
      x3f_directory_entry_t *DE;

      if (NULL == (DE = x3f_get_raw(x3f))) {
	x3f_printf(ERR, "Could not find any matching RAW format\n");
	goto found_error;
      }

      if (X3F_OK != (ret = x3f_load_data(x3f, DE))) {
	x3f_printf(ERR, "Could not load RAW from %s (%s)\n",
		   infile, x3f_err(ret));
	goto found_error;
      }
    }

    if (extract_unconverted_raw) {
      x3f_directory_entry_t *DE;

      if (NULL == (DE = x3f_get_raw(x3f))) {
	x3f_printf(ERR, "Could not find any matching RAW format\n");
	goto found_error;
      }

      if (X3F_OK != (ret = x3f_load_image_block(x3f, DE))) {
	x3f_printf(ERR, "Could not load unconverted RAW from %s (%s)\n",
		   infile, x3f_err(ret));
	goto found_error;
      }
    }

    if (make_paths(infile, outdir, extension[file_type], tmpfile, outfile)) {
      x3f_printf(ERR, "Too large outfile path for infile %s and outdir %s\n",
		 infile, outdir);
      goto found_error;
    }

    unlink(tmpfile);

    /* TODO: Quattro files seem to be already corrected for spatial
       gain. Is that assumption correct? Applying it only worsens the
       result anyhow, so it is disabled by default. */
    sgain =
      apply_sgain == -1 ? x3f->header.version < X3F_VERSION_4_0 : apply_sgain;

    switch (file_type) {
    case META:
      x3f_printf(INFO, "Dump META DATA to %s\n", outfile);
      ret_dump = x3f_dump_meta_data(x3f, tmpfile);
      break;
    case JPEG:
      x3f_printf(INFO, "Dump JPEG to %s\n", outfile);
      ret_dump = x3f_dump_jpeg(x3f, tmpfile);
      break;
    case RAW:
      x3f_printf(INFO, "Dump RAW block to %s\n", outfile);
      ret_dump = x3f_dump_raw_data(x3f, tmpfile);
      break;
    case TIFF:
      x3f_printf(INFO, "Dump RAW as TIFF to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_tiff(x3f, tmpfile,
					   color_encoding,
					   crop, fix_bad, denoise, sgain, wb,
					   compress);
      break;
    case DNG:
      x3f_printf(INFO, "Dump RAW as DNG to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_dng(x3f, tmpfile,
					  fix_bad, denoise, sgain, wb,
					  compress);
      break;
    case PPMP3:
    case PPMP6:
      x3f_printf(INFO, "Dump RAW as PPM to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_ppm(x3f, tmpfile,
					  color_encoding,
					  crop, fix_bad, denoise, sgain, wb,
					  file_type == PPMP6);
      break;
    case HISTOGRAM:
      x3f_printf(INFO, "Dump RAW as CSV histogram to %s\n", outfile);
      ret_dump = x3f_dump_raw_data_as_histogram(x3f, tmpfile,
						color_encoding,
						crop, fix_bad, denoise, sgain, wb,
						log_hist);
      break;
    }

    if (X3F_OK != ret_dump) {
      x3f_printf(ERR, "Could not dump to %s: %s\n", tmpfile, x3f_err(ret_dump));
      errors++;
    } else {
      if (rename(tmpfile, outfile) != 0) {
	x3f_printf(ERR, "Could not rename %s to %s\n", tmpfile, outfile);
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
    x3f_printf(ERR, "No files given\n");
    usage(argv[0]);
  }

  x3f_printf(INFO, "Files processed: %d\terrors: %d\n", files, errors);

  return errors > 0;
}
