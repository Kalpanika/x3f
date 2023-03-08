/* X3F_IMAGE.C
 *
 * Library for access to image data.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_image.h"
#include "x3f_io.h"
#include "x3f_meta.h"
#include "x3f_printf.h"

#include <stdio.h>
#include <assert.h>

/* extern */ int x3f_image_area(x3f_t *x3f, x3f_area16_t *image)
{
  x3f_directory_entry_t *DE = x3f_get_raw(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_image_data_t *ID;
  x3f_huffman_t *HUF;
  x3f_true_t *TRU;
  x3f_area16_t *area = NULL;

  if (!DE) return 0;

  DEH = &DE->header;
  ID = &DEH->data_subsection.image_data;
  HUF = ID->huffman;
  TRU = ID->tru;

  if (HUF != NULL)
    area = &HUF->x3rgb16;

  if (TRU != NULL)
    area = &TRU->x3rgb16;

  if (!area || !area->data) return 0;
  *image = *area;
  image->buf = NULL;		/* cleanup_true/cleanup_huffman is
				   responsible for free() */
  return 1;
}

/* extern */ int x3f_image_area_qtop(x3f_t *x3f, x3f_area16_t *image)
{
  x3f_directory_entry_t *DE = x3f_get_raw(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_image_data_t *ID;
  x3f_quattro_t *Q;

  if (!DE) return 0;

  DEH = &DE->header;
  ID = &DEH->data_subsection.image_data;
  Q = ID->quattro;

  if (!Q || !Q->top16.data) return 0;
  *image = Q->top16;
  image->buf = NULL;		/* cleanup_quattro is responsible for free() */

  return 1;
}

/* extern */ int x3f_crop_area(uint32_t *coord, x3f_area16_t *image,
			       x3f_area16_t *crop)
{
  if (coord[0] > coord[2] || coord[1] > coord[3]) return 0;
  if (coord[2] >= image->columns || coord[3] >= image->rows) return 0;

  crop->data =
    image->data + image->channels*coord[0] + image->row_stride*coord[1];
  crop->columns = coord[2] - coord[0] + 1;
  crop->rows = coord[3] - coord[1] + 1;
  crop->channels = image->channels;
  crop->row_stride = image->row_stride;
  crop->buf = image->buf;

  return 1;
}

/* extern */ int x3f_crop_area8(uint32_t *coord, x3f_area8_t *image,
				x3f_area8_t *crop)
{
  if (coord[0] > coord[2] || coord[1] > coord[3]) return 0;
  if (coord[2] >= image->columns || coord[3] >= image->rows) return 0;

  crop->data =
    image->data + image->channels*coord[0] + image->row_stride*coord[1];
  crop->columns = coord[2] - coord[0] + 1;
  crop->rows = coord[3] - coord[1] + 1;
  crop->channels = image->channels;
  crop->row_stride = image->row_stride;
  crop->buf = image->buf;

  return 1;
}

/* NOTE: The existence of KeepImageArea and, in the case of Quattro,
   layers with different resolution makes coordinate
   transformation pretty complicated.

   For rescale = 0, the origin and resolution of image MUST
   correspond to those of KeepImageArea. image can be bigger but NOT
   smaller than KeepImageArea.

   For rescale = 1, the bounds of image MUST correspond exatly
   to those of KeepImageArea, but their resolutions can be
   different. */

static int x3f_transform_rect_to_keep_image(x3f_t *x3f,
					    x3f_area16_t *image, int rescale,
					    uint32_t *rect)
{
  uint32_t keep[4], keep_cols, keep_rows;

  if (!x3f_get_camf_matrix(x3f, "KeepImageArea", 4, 0, 0, M_UINT, keep))
    return 0;

  keep_cols = keep[2] - keep[0] + 1;
  keep_rows = keep[3] - keep[1] + 1;

  /* Make sure that at least some part of rect is within the bounds of
     KeepImageArea */
  if (rect[0] > keep[2] || rect[1] > keep[3] ||
      rect[2] < keep[0] || rect[3] < keep[1]) {
    x3f_printf(WARN,
	       "CAMF rect (%u,%u,%u,%u) completely out of bounds : "
	       "KeepImageArea (%u,%u,%u,%u)\n",
	       rect[0], rect[1], rect[2], rect[3],
	       keep[0], keep[1], keep[2], keep[3]);
    return 0;
  }

  /* Clip rect to the bouds of KeepImageArea */
  if (rect[0] < keep[0]) rect[0] = keep[0];
  if (rect[1] < keep[1]) rect[1] = keep[1];
  if (rect[2] > keep[2]) rect[2] = keep[2];
  if (rect[3] > keep[3]) rect[3] = keep[3];

  /* Translate rect so coordinates are relative to the origin of
     KeepImageArea */
  rect[0] -= keep[0];
  rect[1] -= keep[1];
  rect[2] -= keep[0];
  rect[3] -= keep[1];

  if (rescale) {
    /* Rescale rect from the resolution of KeepImageArea to that of image */
    rect[0] = rect[0]*image->columns/keep_cols;
    rect[1] = rect[1]*image->rows/keep_rows;
    rect[2] = rect[2]*image->columns/keep_cols;
    rect[3] = rect[3]*image->rows/keep_rows;
  }
  /* Make sure that KeepImageArea is within the bounds of image */
  else if (keep_cols > image->columns || keep_rows > image->rows) {
    x3f_printf(WARN,
	       "KeepImageArea (%u,%u,%u,%u) out of bounds : "
	       "image size (%u,%u)\n",
	       keep[0], keep[1], keep[2], keep[3],
	       image->columns, image->rows);
    return 0;
  }

  return 1;
}

/* extern */ int x3f_get_camf_rect(x3f_t *x3f, char *name,
				   x3f_area16_t *image, int rescale,
				   uint32_t *rect)
{
  if (!x3f_get_camf_matrix(x3f, name, 4, 0, 0, M_UINT, rect))
    return 0;

  return x3f_transform_rect_to_keep_image(x3f, image, rescale, rect);
}

/* extern */ int x3f_crop_area_column(x3f_t *x3f, col_side_t which_side,
				      x3f_area16_t *image, int rescale,
				      x3f_area16_t *crop)
{
  uint32_t rect[4];
  uint32_t column[4];

  if (!x3f_get_camf_matrix(x3f, "DarkShieldColRange", 2, 2, 0, M_UINT, column))
    return 0;

  rect[1] = 0;           /* Cropped automatically later */
  rect[3] = UINT32_MAX;  /* Cropped automatically later */

  if (which_side == COL_SIDE_LEFT) {
    rect[0] = column[0];
    rect[2] = column[1];
  } else if (which_side == COL_SIDE_RIGHT) {
    rect[0] = column[2];
    rect[2] = column[3];
  } else
    return 0;

  if (x3f_transform_rect_to_keep_image(x3f, image, rescale, rect)) {
    int ret = x3f_crop_area(rect, image, crop);
    assert(ret);
    return 1;
  }

  return 0;
}

/* extern */ int x3f_crop_area_camf(x3f_t *x3f, char *name,
				    x3f_area16_t *image, int rescale,
				    x3f_area16_t *crop)
{
  uint32_t rect[4];

  if (!x3f_get_camf_rect(x3f, name, image, rescale, rect)) return 0;
  /* This should not fail as long as x3f_get_camf_rect is implemented
     correctly */
  int ret = x3f_crop_area(rect, image, crop);
  assert(ret);

  return 1;
}

/* extern */ int x3f_crop_area8_camf(x3f_t *x3f, char *name,
				     x3f_area8_t *image, int rescale,
				     x3f_area8_t *crop)
{
  uint32_t rect[4];

  if (!x3f_get_camf_rect(x3f, name, (x3f_area16_t *)image, rescale, rect))
    return 0;
  /* This should not fail as long as x3f_get_camf_rect is implemented
     correctly */
  int ret = x3f_crop_area8(rect, image, crop);
  assert(ret);

  return 1;
}
