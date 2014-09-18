/* X3F_IO_TEST.C - Test of library for accessing X3F Files.
 * 
 * Copyright 2010 (c) - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 * 
 */

#include "x3f_io.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *f_in = NULL;
  x3f_t *x3f = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s infile [outfile [outfile2]]\n", argv[0]);
    return 1;
  }

  f_in = fopen(argv[1], "rb");

  if (f_in == NULL) {
    fprintf(stderr, "Could not open infile %s\n", argv[1]);
    return 1;
  }

  printf("READ THE X3F FILE %s\n", argv[1]);
  x3f = x3f_new_from_file(f_in);

  printf("PRINT THE X3F STRUCTURE\n");
  x3f_print(x3f);

  if (argc > 2) {
    FILE *f_out = fopen(argv[2], "wb");

    if (f_out == NULL) {
      fprintf(stderr, "Could not open outfile %s\n", argv[2]);
    } else {
      printf("WRITE THE X3F FILE %s\n", argv[2]);
      x3f_write_to_file(x3f, f_out);
      fclose(f_out);
    }
  }
  
  if (argc > 3) {
    FILE *f_out = fopen(argv[3], "wb");

    if (f_out == NULL) {
      fprintf(stderr, "Could not open outfile %s\n", argv[3]);
    } else {
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

      printf("PRINT THE X3F STRUCTURE (AGAIN)\n");
      x3f_print(x3f);

      printf("WRITE THE X3F FILE %s\n", argv[3]);
      x3f_write_to_file(x3f, f_out);
      fclose(f_out);
    }
  }

  printf("CLEAN UP\n");
  x3f_delete(x3f);

  fclose(f_in);

  return 0;
}
