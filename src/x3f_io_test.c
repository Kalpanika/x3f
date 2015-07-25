/* X3F_IO_TEST.C
 *
 * Test of library for accessing X3F Files.
 * 
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 * 
 */

#include "x3f_io.h"
#include "x3f_print.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char *progname)
{
  fprintf(stderr,
          "usage: %s [-unpack] [-noprint] <X3F-file>\n",
          progname);
  exit(1);
}

int main(int argc, char *argv[])
{
  FILE *f_in = NULL;
  x3f_t *x3f = NULL;

  int do_unpack_data = 0;
  int do_print_info = 1;

  char *infilename;

  int i;

  for (i=1; i<argc; i++)
    if (!strcmp(argv[i], "-unpack"))
      do_unpack_data = 1;
    else if (!strcmp(argv[i], "-noprint"))
      do_print_info = 0;
    else
      break;			/* Now comes the file name */

  if (argc != i+1) {
    usage(argv[0]);
  }

  infilename = argv[i];
  f_in = fopen(infilename, "rb");

  if (f_in == NULL) {
    fprintf(stderr, "Could not open infile %s\n", infilename);
    return 1;
  }

  printf("READ THE X3F FILE %s\n", argv[1]);
  x3f = x3f_new_from_file(f_in);

  if (do_print_info) {
    printf("PRINT THE SKELETON X3F STRUCTURE\n");
    x3f_print(x3f);
  }

  if (do_unpack_data) {
    printf("LOAD RAW DATA\n");
    x3f_load_data(x3f, x3f_get_raw(x3f));

    printf("LOAD THUMBNAIL DATA\n");
    x3f_load_data(x3f, x3f_get_thumb_plain(x3f));

    printf("LOAD HUFFMAN THUMBNAIL DATA\n");
    x3f_load_data(x3f, x3f_get_thumb_huffman(x3f));

    printf("LOAD JPEG THUMBNAIL DATA\n");
    x3f_load_data(x3f, x3f_get_thumb_jpeg(x3f));

    printf("LOAD CAMF DATA\n");
    x3f_load_data(x3f, x3f_get_camf(x3f));

    printf("LOAD PROP DATA\n");
    x3f_load_data(x3f, x3f_get_prop(x3f));

    if (do_print_info) {
      printf("PRINT THE UNPACKED X3F STRUCTURE\n");
      x3f_print(x3f);
    }
  }

  printf("CLEAN UP\n");
  x3f_delete(x3f);

  fclose(f_in);

  return 0;
}
