/* X3F_DUMP.C
 *
 * Library for writing unprocessed RAW and JPEG data.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_dump.h"
#include "x3f_io.h"

#include <stdio.h>

/* extern */ x3f_return_t x3f_dump_raw_data(x3f_t *x3f,
                                            char *outfilename)
{
  x3f_directory_entry_t *DE = x3f_get_raw(x3f);

  if (DE == NULL) {
    return X3F_ARGUMENT_ERROR;
  } else {
    x3f_directory_entry_header_t *DEH = &DE->header;
    x3f_image_data_t *ID = &DEH->data_subsection.image_data;
    void *data = ID->data;

    if (data == NULL) {
      return X3F_INTERNAL_ERROR;
    } else {
      FILE *f_out = fopen(outfilename, "wb");

      if (f_out == NULL) {
        return X3F_OUTFILE_ERROR;
      } else {
        fwrite(data, 1, DE->input.size, f_out);
        fclose(f_out);
      }
    }
  }

  return X3F_OK;
}

/* extern */ x3f_return_t x3f_dump_jpeg(x3f_t *x3f, char *outfilename)
{
  x3f_directory_entry_t *DE = x3f_get_thumb_jpeg(x3f);

  if (DE == NULL) {
    return X3F_ARGUMENT_ERROR;
  } else {
    x3f_directory_entry_header_t *DEH = &DE->header;
    x3f_image_data_t *ID = &DEH->data_subsection.image_data;
    void *data = ID->data;

    if (data == NULL) {
      return X3F_INTERNAL_ERROR;
    } else {
      FILE *f_out = fopen(outfilename, "wb");

      if (f_out == NULL) {
        return X3F_OUTFILE_ERROR;
      } else {
        fwrite(data, 1, DE->input.size, f_out);
        fclose(f_out);
      }
    }
  }

  return X3F_OK;
}
