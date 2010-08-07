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

typedef enum {NONE, RAW, TIFF, PPM} raw_file_type_t;

static void usage(char *progname)
{
  /* NOTE - the library can write 16 bit PPM also. But
     ... unfortunately almost no tool out there reads 16 bit PPM
     correctly. So - its omitted from the usage printout. */

  fprintf(stderr,
          "usage: %s [-jpg]"
          " [{-raw|-tiff [-gamma <GAMMA> [-min <MIN>] [-max <MAX>]]}]"
          " <file1> ...\n"
          "   -jpg:       Dump embedded JPG\n"
          "   -raw:       Dump RAW area undecoded\n"
          "   -tiff:      Dump RAW as TIFF\n"
          "   -gamma <GAMMA>:  Gamma for scaled TIFF (def=2.2)\n"
          "   -min <MIN>:      Min for scaled TIFF (def=0)\n"
          "   -max <MAX>:      Max for scaled TIFF (def=automatic)",
          progname);
  exit(1);
}

int main(int argc, char *argv[])
{
  int extract_jpg = 0;
  int extract_raw = 0;
  int min = -1;
  int max = -1;
  double gamma = -1.0;
  raw_file_type_t file_type = NONE;
  int files = 0;

  int i;

  for (i=1; i<argc; i++)
    if (!strcmp(argv[i], "-jpg"))
      extract_jpg = 1;
    else if (!strcmp(argv[i], "-raw"))
      extract_raw = 1, file_type = RAW;
    else if (!strcmp(argv[i], "-tiff"))
      extract_raw = 1, file_type = TIFF;
    else if (!strcmp(argv[i], "-ppm"))
      extract_raw = 1, file_type = PPM;
    else if ((!strcmp(argv[i], "-gamma")) && (i+1)<argc)
      gamma = atof(argv[++i]);
    else if ((!strcmp(argv[i], "-min")) && (i+1)<argc)
      min = atoi(argv[++i]);
    else if ((!strcmp(argv[i], "-max")) && (i+1)<argc)
      max = atoi(argv[++i]);
    else if (!strncmp(argv[i], "-", 1))
      usage(argv[0]);
    else
      break;			/* Here starts list of files */

  /* If min or max is set but no gamma - ERROR */
  if (min != -1 || max != -1)
    if (gamma <= 0.0)
      usage(argv[0]);

  /* If gamma is set but no file type that can output gaamma - ERROR */
  if (gamma > 0.0)
    if (file_type != TIFF && file_type != PPM)
      usage(argv[0]);

  for (; i<argc; i++) {
    char *infilename = argv[i];
    FILE *f_in = fopen(infilename, "rb");
    x3f_t *x3f = NULL;

    files++;

    if (f_in == NULL) {
      fprintf(stderr, "Could not open infile %s\n", infilename);
      return 1;
    }

    printf("READ THE X3F FILE %s\n", infilename);
    x3f = x3f_new_from_file(f_in);

    if (extract_jpg) {
      char outfilename[1024];

      x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f));

      strcpy(outfilename, infilename);
      strcat(outfilename, ".jpg");

      printf("Dump JPEG to %s\n", outfilename);
      x3f_dump_jpeg(x3f, outfilename);
    }

    if (extract_raw) {
      char outfilename[1024];

      strcpy(outfilename, infilename);

      switch (file_type) {
      case NONE:
	break;
      case RAW:
	printf("Load RAW block from %s\n", infilename);
	x3f_load_image_block(x3f, x3f_get_raw(x3f));
	strcat(outfilename, ".raw");
	printf("Dump RAW block to %s\n", outfilename);
	x3f_dump_raw_data(x3f, outfilename);
	break;
      case TIFF:
	printf("Load RAW from %s\n", infilename);
	x3f_load_data(x3f, x3f_get_raw(x3f));
	strcat(outfilename, ".tif");
	printf("Dump RAW as TIFF to %s\n", outfilename);
	x3f_dump_raw_data_as_tiff(x3f, outfilename, gamma, min, max);
	break;
      case PPM:
	printf("Load RAW from %s\n", infilename);
	x3f_load_data(x3f, x3f_get_raw(x3f));
	strcat(outfilename, ".ppm");
	printf("Dump RAW as PPM to %s\n", outfilename);
	x3f_dump_raw_data_as_ppm(x3f, outfilename, gamma, min, max);
	break;
      }
    }

    x3f_delete(x3f);

    fclose(f_in);
  }

  if (files == 0)
    usage(argv[0]);

  return 0;
}
