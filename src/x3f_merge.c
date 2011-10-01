/* X3F_MERGE.C - Merging image data from one RAW file to another.
 * 
 * Copyright 2011 (c) - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 * 
 */

#include "x3f_io.h"
#include <stdio.h>

#define CLEANUP(FUNC, VAR) do { if (VAR != NULL) FUNC(VAR); } while(0)

int main(int argc, char *argv[])
{
  FILE *f_in_template = NULL;
  FILE *f_in_images = NULL;
  FILE *f_out = NULL;

  x3f_t *x3f_template = NULL;
  x3f_t *x3f_images = NULL;

  int retval = 0;

  if (argc != 4) {
    fprintf(stderr, "usage: %s in_template_file in_image_file out_file\n",
            argv[0]);
    goto err;
  }

  printf("Open and read infiles\n");

  if (NULL == (f_in_template = fopen(argv[1], "rb"))) {
    fprintf(stderr, "Could not open template infile %s\n", argv[1]);
    goto err;
  }

  if (NULL == (f_in_images = fopen(argv[2], "rb"))) {
    fprintf(stderr, "Could not open images infile %s\n", argv[2]);
    goto err;
  }

  if (NULL == (x3f_template = x3f_new_from_file(f_in_template))) {
    fprintf(stderr, "Could not read template infile %s\n", argv[1]);
    goto err;
  }

  if (NULL == (x3f_images = x3f_new_from_file(f_in_images))) {
    fprintf(stderr, "Could not read template infile %s\n", argv[1]);
    goto err;
  }

  printf("Merge\n");

  if (X3F_OK != x3f_swap_images(x3f_template, x3f_images)) {
    fprintf(stderr, "Could not merge %s and %s\n", argv[1], argv[2]);
    goto err;
  }

  printf("Open and write outfile\n");

  if (NULL == (f_out = fopen(argv[3], "wb"))) {
    fprintf(stderr, "Could not open outfile %s\n", argv[3]);
    goto err;
  }

  if (X3F_OK != x3f_write_to_file(x3f_template, f_out)) {
    fprintf(stderr, "Could not write to outfile %s\n", argv[3]);
    goto err;
  }

  goto cleanup;

 err:

  retval = 1;

 cleanup:

  printf("Cleanup\n");

  CLEANUP(x3f_delete, x3f_template);
  CLEANUP(x3f_delete, x3f_images);

  CLEANUP(fclose, f_in_template);
  CLEANUP(fclose, f_in_images);
  CLEANUP(fclose, f_out);

 return retval;
}
