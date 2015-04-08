#include "x3f_process.h"
#include "x3f_io.h"
#include "x3f_meta.h"
#include "x3f_image.h"
#include "x3f_matrix.h"
#include "x3f_dngtags.h"
#include "x3f_denoise.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <tiffio.h>

static int sum_area(x3f_t *x3f, char *name,
		    x3f_area16_t *image, int rescale, int colors,
		    uint64_t *sum /* in/out */)
{
  x3f_area16_t area;
  int row, col, color;

  if (image->channels < colors) return 0;
  if (!x3f_crop_area_camf(x3f, name, image, rescale, &area)) return 0;

  for (row = 0; row < area.rows; row++)
    for (col = 0; col < area.columns; col++)
      for (color = 0; color < colors; color++)
	sum[color] += (uint64_t)area.data[area.row_stride*row +
					  area.channels*col + color];

  return area.columns*area.rows;
}

static int sum_area_sqdev(x3f_t *x3f, char *name,
			  x3f_area16_t *image, int rescale, int colors,
			  double *mean, double *sum /* in/out */)
{
  x3f_area16_t area;
  int row, col, color;

  if (image->channels < colors) return 0;
  if (!x3f_crop_area_camf(x3f, name, image, rescale, &area)) return 0;

  for (row = 0; row < area.rows; row++)
    for (col = 0; col < area.columns; col++)
      for (color = 0; color < colors; color++) {
	double dev = area.data[area.row_stride*row +
			       area.channels*col + color] - mean[color];
	sum[color] += dev*dev;
      }

  return area.columns*area.rows;
}

static int get_black_level(x3f_t *x3f,
			   x3f_area16_t *image, int rescale, int colors,
			   double *black_level, double *black_dev)
{
  uint64_t *black_sum;
  double *black_sum_sqdev;
  int pixels, i;

  pixels = 0;
  black_sum = alloca(colors*sizeof(uint64_t));
  memset(black_sum, 0, colors*sizeof(uint64_t));
  pixels += sum_area(x3f, "DarkShieldTop", image, rescale, colors,
		     black_sum);
  pixels += sum_area(x3f, "DarkShieldBottom", image, rescale, colors,
		     black_sum);
  if (pixels == 0) return 0;

  for (i=0; i<colors; i++)
    black_level[i] = (double)black_sum[i]/pixels;

  pixels = 0;
  black_sum_sqdev = alloca(colors*sizeof(double));
  memset(black_sum_sqdev, 0, colors*sizeof(double));
  pixels += sum_area_sqdev(x3f, "DarkShieldTop", image, rescale, colors,
			   black_level, black_sum_sqdev);
  pixels += sum_area_sqdev(x3f, "DarkShieldBottom", image, rescale, colors,
			   black_level, black_sum_sqdev);
  if (pixels == 0) return 0;

  for (i=0; i<colors; i++)
    black_dev[i] = sqrt(black_sum_sqdev[i]/pixels);

  return 1;
}

static void get_raw_neutral(double *raw_to_xyz, double *raw_neutral)
{
  double d65_xyz[3] = {0.95047, 1.00000, 1.08883};
  double xyz_to_raw[9];

  x3f_3x3_inverse(raw_to_xyz, xyz_to_raw);
  x3f_3x3_3x1_mul(xyz_to_raw, d65_xyz, raw_neutral);
}

static int get_gain(x3f_t *x3f, char *wb, double *gain)
{
  double cam_to_xyz[9], wb_correction[9], gain_fact[3];

  if (x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceGains", wb, 3, 0, gain) ||
      x3f_get_camf_matrix_for_wb(x3f, "DP1_WhiteBalanceGains", wb, 3, 0, gain));
  else if (x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceIlluminants", wb,
				      3, 3, cam_to_xyz) &&
	   x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceCorrections", wb,
				      3, 3, wb_correction)) {
    double raw_to_xyz[9], raw_neutral[3];

    x3f_3x3_3x3_mul(wb_correction, cam_to_xyz, raw_to_xyz);
    get_raw_neutral(raw_to_xyz, raw_neutral);
    x3f_3x1_invert(raw_neutral, gain);
  }
  else
    return 0;

  if (x3f_get_camf_float_vector(x3f, "SensorAdjustmentGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);

  if (x3f_get_camf_float_vector(x3f, "TempGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);

  if (x3f_get_camf_float_vector(x3f, "FNumberGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);

  printf("gain\n");
  x3f_3x1_print(gain);

  return 1;
}

static int get_bmt_to_xyz(x3f_t *x3f, char *wb, double *bmt_to_xyz)
{
  double cc_matrix[9], cam_to_xyz[9], wb_correction[9];

  if (x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceColorCorrections", wb,
				 3, 3, cc_matrix) ||
      x3f_get_camf_matrix_for_wb(x3f, "DP1_WhiteBalanceColorCorrections", wb,
				 3, 3, cc_matrix)) {
    double srgb_to_xyz[9];

    x3f_sRGB_to_XYZ(srgb_to_xyz);
    x3f_3x3_3x3_mul(srgb_to_xyz, cc_matrix, bmt_to_xyz);
  }
  else if (x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceIlluminants", wb,
				      3, 3, cam_to_xyz) &&
	   x3f_get_camf_matrix_for_wb(x3f, "WhiteBalanceCorrections", wb,
				      3, 3, wb_correction)) {
    double raw_to_xyz[9], raw_neutral[3], raw_neutral_mat[9];

    x3f_3x3_3x3_mul(wb_correction, cam_to_xyz, raw_to_xyz);
    get_raw_neutral(raw_to_xyz, raw_neutral);
    x3f_3x3_diag(raw_neutral, raw_neutral_mat);
    x3f_3x3_3x3_mul(raw_to_xyz, raw_neutral_mat, bmt_to_xyz);
  }
  else
    return 0;

  printf("bmt_to_xyz\n");
  x3f_3x3_print(bmt_to_xyz);

  return 1;
}

static int get_raw_to_xyz(x3f_t *x3f, char *wb, double *raw_to_xyz)
{
  double bmt_to_xyz[9], gain[9], gain_mat[9];

  if (!get_gain(x3f, wb, gain)) return 0;
  if (!get_bmt_to_xyz(x3f, wb, bmt_to_xyz)) return 0;

  x3f_3x3_diag(gain, gain_mat);
  x3f_3x3_3x3_mul(bmt_to_xyz, gain_mat, raw_to_xyz);

  printf("raw_to_xyz\n");
  x3f_3x3_print(raw_to_xyz);

  return 1;
}

static int get_raw_to_xyz_noconvert(x3f_t *x3f, char *wb, double *raw_to_xyz)
{
  double adobergb_to_xyz[9], gain[9], gain_mat[9];

  if (!get_gain(x3f, wb, gain)) return 0;
  /* TODO: assuming working space to be Adobe RGB. Is that acceptable? */
  x3f_AdobeRGB_to_XYZ(adobergb_to_xyz);

  x3f_3x3_diag(gain, gain_mat);
  x3f_3x3_3x3_mul(adobergb_to_xyz, gain_mat, raw_to_xyz);

  printf("raw_to_xyz\n");
  x3f_3x3_print(raw_to_xyz);

  return 1;
}

static double lens_position(double focal_length, double object_distance)
{
  return 1.0/(1.0/focal_length - 1.0/object_distance);
}

static double get_focal_length(x3f_t *x3f)
{
  char *flength;
  double focal_length;

  if (x3f_get_prop_entry(x3f, "FLENGTH", &flength))
    focal_length = atof(flength);
  else {
    focal_length = 30.0;
    fprintf(stderr, "WARNING: could not get focal length, assuming %g mm\n",
	    focal_length);
  }

  return focal_length;
}

static double get_object_distance(x3f_t *x3f)
{
  double object_distance;

  if (x3f_get_camf_float(x3f, "ObjectDistance", &object_distance))
    object_distance *= 10.0;	/* Convert cm to mm */
  else {
    object_distance = INFINITY;
    fprintf(stderr, "WARNING: could not get object distance, assuming %g mm\n",
	    object_distance);
  }

  return object_distance;
}

/* Get the minimum object distance (MOD)  */
static double get_MOD(x3f_t *x3f)
{
  int32_t lens_information;
  double mod;

  if (!x3f_get_camf_signed(x3f, "LensInformation", &lens_information))
    lens_information = -1;

  /* TODO: is there any better way of obtaining MOD? */
  switch (lens_information) {
  case 1003:			/* DP1 Merrill */
    mod = 200.0;
    break;
  case 1004:			/* DP2 Merrill */
    mod = 280.0;
    break;
  case 1005:			/* DP3 Merrill */
    mod = 226.0;
    break;
  default:
    mod = 280.0;
    fprintf(stderr, "WARNING: could not get MOD, assuming %g mm\n", mod);
  }

  return mod;
}

typedef struct {
  double weight;

  uint32_t *gain;
  double mingain, delta;
} spatial_gain_corr_merrill_t;

typedef struct {
  double *gain;		    /* final gain (interpolated if necessary) */
  int malloc;		    /* 1 if gain is allocated with malloc() */

  int rows, cols;
  int rowoff, coloff, rowpitch, colpitch;
  int chan, channels;

  spatial_gain_corr_merrill_t mgain[4]; /* raw Merrill-type gains */
  int mgain_num;
} spatial_gain_corr_t;

static int get_merrill_type_gains_table(x3f_t *x3f, char *name, char *chan,
					uint32_t **mgain, int *rows, int *cols,
					double *mingain, double *delta)
{
  char table[32], *val;
  int rows_tmp, cols_tmp;

  sprintf(table, "GainsTable%s", chan);
  if (!x3f_get_camf_property(x3f, name, table, &val) ||
      !x3f_get_camf_matrix_var(x3f, val, &rows_tmp, &cols_tmp, NULL,
			       M_UINT, (void **)mgain) ||
      (*rows != -1 && *rows != rows_tmp) ||
      (*cols != -1 && *cols != cols_tmp))
    return 0;
  *rows = rows_tmp;
  *cols = cols_tmp;

  sprintf(table, "MinGains%s", chan);
  if (!x3f_get_camf_property(x3f, name, table, &val)) return 0;
  *mingain = atof(val);

  sprintf(table, "Delta%s", chan);
  if (!x3f_get_camf_property(x3f, name, table, &val)) return 0;
  *delta = atof(val);

  return 1;
}

typedef struct merrill_spatial_gain_s {
  char *name;
  double x,y;
  struct merrill_spatial_gain_s *next;
} merrill_spatial_gain_t;

#define MAXCORR 6 /* Quattro HP: R, G, B0, B1, B2, B3 */

static int get_merrill_type_spatial_gain(x3f_t *x3f, int hp_flag,
					 spatial_gain_corr_t *corr)
{
  char **include_blocks, **include_blocks_val;
  uint32_t include_blocks_num;
  double capture_aperture;
  double *spatial_gain_fstop;
  int num_fstop;
  int corr_num = 3;

  merrill_spatial_gain_t *gains = NULL, *g;
  merrill_spatial_gain_t *q_closest[4] = {NULL, NULL, NULL, NULL};
  double q_closest_dx[4] = {INFINITY, -INFINITY, -INFINITY, INFINITY};
  double q_closest_dy[4] = {INFINITY, INFINITY, -INFINITY, -INFINITY};
  double q_closest_d2[4] = {INFINITY, INFINITY, INFINITY, INFINITY};
  double q_weight_x[4], q_weight_y[4], q_weight[4];
  double x,y;
  int i;

  if (!x3f_get_camf_float(x3f, "CaptureAperture", &capture_aperture)) return 0;

  if (!x3f_get_camf_property_list(x3f, "IncludeBlocks",
				  &include_blocks, &include_blocks_val,
				  &include_blocks_num))
    return 0;

  if (hp_flag) {		/* Quattro HP */
    if (!x3f_get_camf_matrix_var(x3f, "SpatialGainHP_Fstop",
				 &num_fstop, NULL, NULL,
				 M_FLOAT, (void **)&spatial_gain_fstop))
      return 0;
    corr_num = 6; /* R, G, B0, B1, B2, B3 */

    for (i=0; i<include_blocks_num; i++) {
      char **names, **values;
      uint32_t num, aperture_index;
      char dummy;

      if (sscanf(include_blocks[i], "SpatialGainHPProps_%d%c",
		 &aperture_index, &dummy) == 1 &&
	  x3f_get_camf_property_list(x3f, include_blocks[i],
				     &names, &values, &num) &&
	  aperture_index >= 0 && aperture_index < num_fstop) {
	g = alloca(sizeof(merrill_spatial_gain_t));
	g->name = include_blocks[i];
	g->x = 1.0/spatial_gain_fstop[aperture_index];
	g->y = 0.0;		/* unused */
	g->next = gains;
	gains = g;
      }
    }

    x = 1.0/capture_aperture;
    y = 0.0;			/* unused */
  }
  else {			/* Merrill and Quattro */
    if (x3f_get_camf_matrix_var(x3f, "SpatialGain_Fstop",
				&num_fstop, NULL, NULL,
				M_FLOAT, (void **)&spatial_gain_fstop))
      for (i=0; i<include_blocks_num; i++) {
	char **names, **values;
	uint32_t num, aperture_index;
	char focus_distance[4], dummy;

	if (sscanf(include_blocks[i], "SpatialGainsProps_%d_%3s%c",
		   &aperture_index, focus_distance, &dummy) == 2 &&
	    x3f_get_camf_property_list(x3f, include_blocks[i],
				       &names, &values, &num) &&
	    aperture_index >= 0 && aperture_index < num_fstop) {
	  double lenspos;

	  if (!strcmp(focus_distance, "INF"))
	    lenspos = lens_position(get_focal_length(x3f), INFINITY);
	  else if (!strcmp(focus_distance, "MOD"))
	    lenspos = lens_position(get_focal_length(x3f), get_MOD(x3f));
	  else continue;

	  g = alloca(sizeof(merrill_spatial_gain_t));
	  g->name = include_blocks[i];
	  g->x = 1.0/spatial_gain_fstop[aperture_index];
	  g->y = lenspos;
	  g->next = gains;
	  gains = g;
	}
      }
    else
      for (i=0; i<include_blocks_num; i++) {
	char **names, **values;
	uint32_t num;
	double aperture, lenspos;
	char dummy;

	if (sscanf(include_blocks[i], "SpatialGainsProps_%lf_%lf%c",
		   &aperture, &lenspos, &dummy) == 2 &&
	    x3f_get_camf_property_list(x3f, include_blocks[i],
				       &names, &values, &num)) {
	  g = alloca(sizeof(merrill_spatial_gain_t));
	  g->name = include_blocks[i];
	  g->x = 1.0/aperture;
	  g->y = lenspos;
	  g->next = gains;
	  gains = g;
	}
      }

    /* TODO: doing bilinear interpolation with respect to 1/aperture
       and lens position. Is that correct? */
    x = 1.0/capture_aperture;
    y = lens_position(get_focal_length(x3f), get_object_distance(x3f));
  }

  for (g=gains; g; g=g->next) {
    double dx = g->x - x;
    double dy = g->y - y;
    double d2 = dx*dx + dy*dy;
    int q;

    if      (dx > 0.0 && dy > 0.0) q = 0;
    else if (dx > 0.0)             q = 3;
    else if (dy > 0.0)             q = 1;
    else                           q = 2;

    if (d2 < q_closest_d2[q]) {
      q_closest[q] = g;
      q_closest_dx[q] = dx;
      q_closest_dy[q] = dy;
      q_closest_d2[q] = d2;
    }
  }

  /* TODO: bilinear interpolation requires that the points be laid out
     in a more or less rectilinear grid. Is that assumption good
     enough? It appears to be valid for the current cameras. */
  q_weight_x[0] = q_closest_dx[1]/(q_closest_dx[1] - q_closest_dx[0]);
  q_weight_x[1] = q_closest_dx[0]/(q_closest_dx[0] - q_closest_dx[1]);
  q_weight_x[2] = q_closest_dx[3]/(q_closest_dx[3] - q_closest_dx[2]);
  q_weight_x[3] = q_closest_dx[2]/(q_closest_dx[2] - q_closest_dx[3]);

  q_weight_y[0] = q_closest_dy[3]/(q_closest_dy[3] - q_closest_dy[0]);
  q_weight_y[1] = q_closest_dy[2]/(q_closest_dy[2] - q_closest_dy[1]);
  q_weight_y[2] = q_closest_dy[1]/(q_closest_dy[1] - q_closest_dy[2]);
  q_weight_y[3] = q_closest_dy[0]/(q_closest_dy[0] - q_closest_dy[3]);

  printf("x = %f y = %f\n", x, y);
  for (i=0; i<4; i++) {
    if (q_closest[i])
      printf("q = %d name = %s x = %f y = %f\n",
	     i, q_closest[i]->name, q_closest[i]->x, q_closest[i]->y);
    else
      printf("q = %d name = NULL\n", i);
    printf("q = %d dx = %f dy = %f d2 = %f wx = %f wy = %f\n",
	   i, q_closest_dx[i], q_closest_dy[i], q_closest_d2[i],
	   q_weight_x[i], q_weight_y[i]);
  }

  for (i=0; i<4; i++) {
    if (isnan(q_weight_x[i])) q_weight_x[i] = 1.0;
    if (isnan(q_weight_y[i])) q_weight_y[i] = 1.0;
    q_weight[i] = q_weight_x[i]*q_weight_y[i];
  }

  for (i=0; i<4; i++)
    printf("q = %d name = %s w = %f\n",
	   i, q_closest[i] ? q_closest[i]->name : "NULL", q_weight[i]);

  for (i=0; i<corr_num; i++) {
    spatial_gain_corr_t *c = &corr[i];
    c->gain = NULL;		/* No interploated gains present */
    c->malloc = 0;

    c->rows = c->cols = -1;
    c->rowoff = c->coloff = 0;
    c->rowpitch = c->colpitch = 1;
    c->chan = i;
    c->channels = 1;

    c->mgain_num = 0;
  }

  if (hp_flag) {
    corr[2].rowoff = 0;
    corr[2].coloff = 0;
    corr[2].rowpitch = corr[2].colpitch = 2;
    corr[2].chan = 2;

    corr[3].rowoff = 0;
    corr[3].coloff = 1;
    corr[3].rowpitch = corr[3].colpitch = 2;
    corr[3].chan = 2;

    corr[4].rowoff = 1;
    corr[4].coloff = 0;
    corr[4].rowpitch = corr[4].colpitch = 2;
    corr[4].chan = 2;

    corr[5].rowoff = 1;
    corr[5].coloff = 1;
    corr[5].rowpitch = corr[5].colpitch = 2;
    corr[5].chan = 2;
  }

  for (i=0; i<4; i++) {
    char *name;
    char *channels_normal[3] = {"R", "G", "B"};
    char *channels_hp[6] = {"R", "G", "B0", "B1", "B2", "B3"};
    char **channels = hp_flag ? channels_hp : channels_normal;
    int j;

    if (!q_closest[i]) continue;
    name = q_closest[i]->name;

    for (j=0; j<corr_num; j++) {
      spatial_gain_corr_t *c = &corr[j];
      spatial_gain_corr_merrill_t *m = &c->mgain[c->mgain_num++];

      m->weight = q_weight[i];
      if (!get_merrill_type_gains_table(x3f, name, channels[j],
					&m->gain, &c->rows, &c->cols,
					&m->mingain, &m->delta))
	return 0;
    }
  }

  for (i=0; i<corr_num; i++)
    if (corr[i].mgain_num == 0) return 0;

  return corr_num;
}

static int get_interp_merrill_type_spatial_gain(x3f_t *x3f, int hp_flag,
						spatial_gain_corr_t *corr)
{
  int corr_num = get_merrill_type_spatial_gain(x3f, hp_flag, corr);
  int i;

  for (i=0; i<corr_num; i++) {
    spatial_gain_corr_t *c = &corr[i];
    int j, num = c->rows*c->cols*c->channels;

    c->gain = malloc(num*sizeof(double));
    c->malloc = 1;

    for (j=0; j<num; j++) {
      int g;

      c->gain[j] = 0.0;
      for (g=0; g < c->mgain_num; g++) {
	spatial_gain_corr_merrill_t *m = &c->mgain[g];
	c->gain[j] += m->weight*(m->mingain + m->delta*m->gain[j]);
      }
    }
  }

  return corr_num;
}

static int get_classic_spatial_gain(x3f_t *x3f, char *wb,
				    spatial_gain_corr_t *corr)
{
  char *gain_name;

  if ((x3f_get_camf_property(x3f, "SpatialGainTables", wb, &gain_name) &&
       x3f_get_camf_matrix_var(x3f, gain_name,
			       &corr->rows, &corr->cols, &corr->channels,
			       M_FLOAT, (void **)&corr->gain)) ||
      x3f_get_camf_matrix_var(x3f, "SpatialGain",
			      &corr->rows, &corr->cols, &corr->channels,
			      M_FLOAT, (void **)&corr->gain)) {
    corr->malloc = 0;
    corr->rowoff = corr->coloff = 0;
    corr->rowpitch = corr->colpitch = 1;
    corr->chan = 0;
    corr->mgain_num = 0;

    return 1;
  }

  return 0;
}

static int get_spatial_gain(x3f_t *x3f, char *wb, spatial_gain_corr_t *corr)
{
  int corr_num = 0;

  corr_num += get_interp_merrill_type_spatial_gain(x3f, 0, &corr[corr_num]);
  if (corr_num == 0)
    corr_num += get_classic_spatial_gain(x3f, wb, &corr[corr_num]);

  return corr_num;
}

static void cleanup_spatial_gain(spatial_gain_corr_t *corr, int corr_num)
{
  int i;

  for (i=0; i<corr_num; i++)
    if (corr[i].malloc) free(corr[i].gain);
}

static double calc_spatial_gain(spatial_gain_corr_t *corr, int corr_num,
				int row, int col, int chan, int rows, int cols)
{
  double gain = 1.0;
  double rrel = (double)row/rows;
  double crel = (double)col/cols;
  int i;

  for (i=0; i<corr_num; i++) {
    spatial_gain_corr_t *c = &corr[i];
    double rc, cc;
    int ri, ci;
    double rf, cf;
    double *r1, *r2;
    int co1, co2;
    double gr1, gr2;
    int ch = chan - c->chan;

    if (ch < 0 || ch >= c->channels) continue;

    if (row%c->rowpitch != c->rowoff) continue;
    if (col%c->colpitch != c->coloff) continue;

    rc = rrel*(c->rows-1);
    ri = (int)floor(rc);
    rf = rc - ri;

    cc = crel*(c->cols-1);
    ci = (int)floor(cc);
    cf = cc - ci;

    if (ri < 0)
      r1 = r2 = &c->gain[0];
    else if (ri >= c->rows-1)
      r1 = r2 = &c->gain[(c->rows-1)*c->cols*c->channels];
    else {
      r1 = &c->gain[ri*c->cols*c->channels];
      r2 = &c->gain[(ri+1)*c->cols*c->channels];
    }

    if (ci < 0)
      co1 = co2 = ch;
    if (ci >= c->cols-1)
      co1 = co2 = (c->cols-1)*c->channels + ch;
    else {
      co1 = ci*c->channels + ch;
      co2 = (ci+1)*c->channels + ch;
    }

    gr1 = r1[co1] + cf*(r1[co2]-r1[co1]);
    gr2 = r2[co1] + cf*(r2[co2]-r2[co1]);

    gain *= gr1 + rf*(gr2-gr1);
  }

  return gain;
}

/* x3f_denoise expects a 14-bit image since rescaling by a factor of 4
   takes place internally. */
#define INTERMEDIATE_DEPTH 14
#define INTERMEDIATE_UNIT ((1<<INTERMEDIATE_DEPTH) - 1)
#define INTERMEDIATE_BIAS_FACTOR 4.0

static int get_max_intermediate(x3f_t *x3f, char *wb,
				double intermediate_bias,
				uint32_t *max_intermediate)
{
  double gain[3], maxgain = 0.0;
  int i;

  if (!get_gain(x3f, wb, gain)) return 0;

  /* Cap the gains to 1.0 to avoid clipping */
  for (i=0; i<3; i++)
    if (gain[i] > maxgain) maxgain = gain[i];
  for (i=0; i<3; i++)
    max_intermediate[i] =
      (int32_t)round(gain[i]*(INTERMEDIATE_UNIT - intermediate_bias)/maxgain +
		     intermediate_bias);

  return 1;
}

static int get_intermediate_bias(x3f_t *x3f, char *wb,
				 double *black_level, double *black_dev,
				 double *intermediate_bias)
{
  uint32_t max_raw[3], max_intermediate[3];
  int i;

  if (!x3f_get_max_raw(x3f, max_raw)) return 0;
  if (!get_max_intermediate(x3f, wb, 0, max_intermediate)) return 0;

  *intermediate_bias = 0.0;
  for (i=0; i<3; i++) {
    double bias = INTERMEDIATE_BIAS_FACTOR * black_dev[i] *
      max_intermediate[i] / (max_raw[i] - black_level[i]);
    if (bias > *intermediate_bias) *intermediate_bias = bias;
  }

  return 1;
}

typedef struct bad_pixel_s {
  int c, r;
  struct bad_pixel_s *prev, *next;
} bad_pixel_t;

/* Address pixel at column _c and row _r */
#define _PN(_c, _r, _cs) ((_r)*(_cs) + (_c))

/* Test if a pixel (_c,_r) is within a rectancle */
#define _INB(_c, _r, _cs, _rs)					\
  ((_c) >= 0 && (_c) < (_cs) && (_r) >= 0 && (_r) < (_rs))

/* Test if a pixel has been marked in the bad pixel vector */
#define TEST_PIX(_vec, _c, _r, _cs, _rs)				\
  (_INB((_c), (_r), (_cs), (_rs)) ?					\
   (_vec)[_PN((_c), (_r), (_cs)) >> 5] &				\
   1 << (_PN((_c), (_r), (_cs)) & 0x1f) : 1)

/* Mark the pixel, in the bad pixel vector and the bad pixel list */
#define MARK_PIX(_list, _vec, _c, _r, _cs, _rs)				\
  do {									\
    if (!TEST_PIX((_vec), (_c), (_r), (_cs), (_rs))) {			\
      bad_pixel_t *_p = malloc(sizeof(bad_pixel_t));			\
      _p->c = (_c);							\
      _p->r = (_r);							\
      _p->prev = NULL;							\
      _p->next = (_list);						\
      if (_list) (_list)->prev = (_p);					\
      (_list) = _p;							\
      (_vec)[_PN((_c), (_r), (_cs)) >> 5] |=				\
	1 << (_PN((_c), (_r), (_cs)) & 0x1f);				\
    }									\
    else if (!_INB((_c), (_r), (_cs), (_rs)))				\
      fprintf(stderr,							\
	      "WARNING: bad pixel (%u,%u) out of bounds : (%u,%u)\n",	\
	      (_c), (_r), (_cs), (_rs));				\
  } while (0)

/* Clear the mark in the bad pixel vector */
#define CLEAR_PIX(_vec, _c, _r, _cs, _rs)				\
  do {									\
    assert(_INB((_c), (_r), (_cs), (_rs)));				\
    _vec[_PN((_c), (_r), (_cs)) >> 5] &=				\
      ~(1 << (_PN((_c), (_r), (_cs)) & 0x1f));				\
  } while (0)

static void interpolate_bad_pixels(x3f_t *x3f, x3f_area16_t *image, int colors)
{
  bad_pixel_t *bad_pixel_list = NULL;
  uint32_t *bad_pixel_vec = calloc((image->rows*image->columns + 31)/32,
				   sizeof(uint32_t));
  int row, col, color, i;
  uint32_t *bpf23;
  int bpf23_len;
  int stat_pass = 0;		/* Statistics */
  int fix_corner = 0;		/* By default, do not accept corners */

  /* BEGIN - collecting bad pixels. This part reads meta data and
     collects all bad pixels both in the list 'bad_pixel_list' and the
     vector 'bad_pixel_vec' */

  if (colors == 3) {
    uint32_t keep[4], hpinfo[4], *bp, *bpf20;
    int bp_num, bpf20_rows, bpf20_cols;

    if (x3f_get_camf_matrix(x3f, "KeepImageArea", 4, 0, 0, M_UINT, keep) &&
	x3f_get_camf_matrix_var(x3f, "BadPixels", &bp_num, NULL, NULL,
				M_UINT, (void **)&bp))
      for (i=0; i < bp_num; i++)
	MARK_PIX(bad_pixel_list, bad_pixel_vec,
		 ((bp[i] & 0x000fff00) >> 8) - keep[0],
		 ((bp[i] & 0xfff00000) >> 20) - keep[1],
		 image->columns, image->rows);

    /* NOTE: the numbers of rows and cols in this matrix are
       interchanged due to bug in camera firmware */
    if (x3f_get_camf_matrix_var(x3f, "BadPixelsF20",
				&bpf20_cols, &bpf20_rows, NULL,
				M_UINT, (void **)&bpf20) && bpf20_cols == 3)
      for (row=0; row < bpf20_rows; row++)
	MARK_PIX(bad_pixel_list, bad_pixel_vec,
		 bpf20[3*row + 1], bpf20[3*row + 0],
		 image->columns, image->rows);

    /* NOTE: the numbers of rows and cols in this matrix are
       interchanged due to bug in camera firmware
       TODO: should Jpeg_BadClutersF20 really be used for RAW? It works
       though. */
    if (x3f_get_camf_matrix_var(x3f, "Jpeg_BadClusters",
				&bpf20_cols, &bpf20_rows, NULL,
				M_UINT, (void **)&bpf20) && bpf20_cols == 3)
      for (row=0; row < bpf20_rows; row++)
	MARK_PIX(bad_pixel_list, bad_pixel_vec,
		 bpf20[3*row + 1], bpf20[3*row + 0],
		 image->columns, image->rows);

    /* TODO: should those really be interpolated over, or should they be
       rescaled instead? */
    if (x3f_get_camf_matrix(x3f, "HighlightPixelsInfo", 2, 2, 0, M_UINT,
			    hpinfo))
      for (row = hpinfo[1]; row < image->rows; row += hpinfo[3])
	for (col = hpinfo[0]; col < image->columns; col += hpinfo[2])
	  MARK_PIX(bad_pixel_list, bad_pixel_vec,
		   col, row, image->columns, image->rows);
  } /* colors == 3 */

  if ((colors == 1 && x3f_get_camf_matrix_var(x3f, "BadPixelsLumaF23",
					      &bpf23_len, NULL, NULL,
					      M_UINT, (void **)&bpf23)) ||
      (colors == 3 && x3f_get_camf_matrix_var(x3f, "BadPixelsChromaF23",
					      &bpf23_len, NULL, NULL,
					      M_UINT, (void **)&bpf23)))
    for (i=0, row=-1; i < bpf23_len; i++)
      if (row == -1) row = bpf23[i];
      else if (bpf23[i] == 0) row = -1;
      else {MARK_PIX(bad_pixel_list, bad_pixel_vec,
		     bpf23[i], row,
		     image->columns, image->rows); i++;}

  /* END - collecting bad pixels */


  /* BEGIN - fixing bad pixels. This part fixes all bad pixels
     collected in the list 'bad_pixel_list', using the mirror data in
     the vector 'bad_pixel_vec'.  This is made in passes. In each pass
     all pixels that can be interpolated are interpolated and also
     removed from the list of bad pixels.  Eventually the list of bad
     pixels is going to be empty. */

  while (bad_pixel_list) {
    bad_pixel_t *p, *pn;
    bad_pixel_t *fixed = NULL;	/* Contains all, in this pass, fixed pixels */
    struct {
      int all_four, two_linear, two_corner, left; /* Statistics */
    } stats = {0,0,0,0};

    /* Iterate over all pixels in the bad pixel list, in this pass */
    for (p=bad_pixel_list; p && (pn=p->next, 1); p=pn) {
      uint16_t *outp =
	&image->data[p->r*image->row_stride + p->c*image->channels];
      uint16_t *inp[4] = {NULL, NULL, NULL, NULL};
      int num = 0;

      /* Collect status of neighbor pixels */
      if (!TEST_PIX(bad_pixel_vec, p->c - 1, p->r, image->columns, image->rows))
	num++, inp[0] =
	  &image->data[p->r*image->row_stride + (p->c - 1)*image->channels];
      if (!TEST_PIX(bad_pixel_vec, p->c + 1, p->r, image->columns, image->rows))
	num++, inp[1] =
	  &image->data[p->r*image->row_stride + (p->c + 1)*image->channels];
      if (!TEST_PIX(bad_pixel_vec, p->c, p->r - 1, image->columns, image->rows))
	num++, inp[2] =
	  &image->data[(p->r - 1)*image->row_stride + p->c*image->channels];
      if (!TEST_PIX(bad_pixel_vec, p->c, p->r + 1, image->columns, image->rows))
	num++, inp[3] =
	  &image->data[(p->r + 1)*image->row_stride + p->c*image->channels];

      /* Test if interpolation is possible ... */
      if (inp[0] && inp[1] && inp[2] && inp[3])
	/* ... all four neighbors are OK */
	stats.all_four++;
      else if (inp[0] && inp[1])
	/* ... left and right are OK */
	inp[2] = inp[3] = NULL, num = 2, stats.two_linear++;
      else if (inp[2] && inp[3])
	/* ... above and under are OK */
	inp[0] = inp[1] = NULL, num = 2, stats.two_linear++;
      else if (fix_corner && num == 2)
	/* ... corner (plus nothing else to do) are OK */
	stats.two_corner++;
      else
	/* ... nope - it was not possible. Look at next without doing
	   interpolation.  */
	{stats.left++; continue;};

      /* Interpolate the actual pixel */
      for (color=0; color < colors; color++) {
	uint32_t sum = 0;
	for (i=0; i<4; i++)
	  if (inp[i]) sum += inp[i][color];
	outp[color] = (sum + num/2)/num;
      }

      /* Remove p from bad_pixel_list */
      if (p->prev) p->prev->next  = p->next;
      else         bad_pixel_list = p->next;
      if (p->next) p->next->prev = p->prev;

      /* Add p to fixed list */
      p->prev = NULL;
      p->next = fixed;
      fixed = p;
    }

    printf("Bad pixels pass %d: %d fixed (%d all_four, %d linear, %d corner), %d left\n",
	   stat_pass,
	   stats.all_four + stats.two_linear + stats.two_corner,
	   stats.all_four,
	   stats.two_linear,
	   stats.two_corner,
	   stats.left);

    if (!fixed) {
      /* If nothing else to do, accept corners */
      if (!fix_corner) fix_corner = 1;
      else {
	fprintf(stderr,	"WARNING: Failed to interpolate %d bad pixels\n",
		stats.left);
	fixed = bad_pixel_list;	/* Free remaining list entries */
	bad_pixel_list = NULL;	/* Force termination */
      }
    }

    /* Clear the bad pixel vector and free the list */
    for (p=fixed; p && (pn=p->next, 1); p=pn) {
      CLEAR_PIX(bad_pixel_vec, p->c, p->r, image->columns, image->rows);
      free(p);
    }

    stat_pass++;
  }

  /* END - fixing bad pixels */

  free(bad_pixel_vec);
}

typedef struct {
  double black[3];
  uint32_t white[3];
} image_levels_t;

static int preprocess_data(x3f_t *x3f, char *wb, image_levels_t *ilevels)
{
  x3f_area16_t image, qtop;
  int row, col, color;
  uint32_t max_raw[3];
  double scale[3], black_level[3], black_dev[3], intermediate_bias;
  int quattro = x3f_image_area_qtop(x3f, &qtop);
  int colors_in = quattro ? 2 : 3;

  if (!x3f_image_area(x3f, &image) || image.channels < 3) return 0;
  if (quattro && (qtop.channels < 1 ||
		  qtop.rows < 2*image.rows || qtop.columns < 2*image.columns))
    return 0;

  if (!get_black_level(x3f, &image, 1, colors_in, black_level, black_dev) ||
      (quattro && !get_black_level(x3f, &qtop, 0, 1,
				   &black_level[2], &black_dev[2]))) {
    fprintf(stderr, "Could not get black level\n");
    return 0;
  }
  printf("black_level = {%g,%g,%g}, black_dev = {%g,%g,%g}\n",
	 black_level[0], black_level[1], black_level[2],
	 black_dev[0], black_dev[1], black_dev[2]);

  if (!x3f_get_max_raw(x3f, max_raw)) {
    fprintf(stderr, "Could not get maximum RAW level\n");
    return 0;
  }
  printf("max_raw = {%u,%u,%u}\n", max_raw[0], max_raw[1], max_raw[2]);

  if (!get_intermediate_bias(x3f, wb, black_level, black_dev,
			     &intermediate_bias)) {
    fprintf(stderr, "Could not get intermediate bias\n");
    return 0;
  }
  printf("intermediate_bias = %g\n", intermediate_bias);
  ilevels->black[0] = ilevels->black[1] = ilevels->black[2] = intermediate_bias;

  if (!get_max_intermediate(x3f, wb, intermediate_bias, ilevels->white)) {
    fprintf(stderr, "Could not get maximum intermediate level\n");
    return 0;
  }
  printf("max_intermediate = {%u,%u,%u}\n",
	 ilevels->white[0], ilevels->white[1], ilevels->white[2]);

  for (color = 0; color < 3; color++)
    scale[color] = (ilevels->white[color] - ilevels->black[color]) /
      (max_raw[color] - black_level[color]);

  /* Preprocess image data (HUF/TRU->x3rgb16) */
  for (row = 0; row < image.rows; row++)
    for (col = 0; col < image.columns; col++)
      for (color = 0; color < colors_in; color++) {
	uint16_t *valp =
	  &image.data[image.row_stride*row + image.channels*col + color];
	int32_t out =
	  (int32_t)round(scale[color] * (*valp - black_level[color]) +
			 ilevels->black[color]);

	if (out < 0) *valp = 0;
	else if (out > 65535) *valp = 65535;
	else *valp = out;
      }

  if (quattro) {
    /* Preprocess and downsample Quattro top layer (Q->top16) */
    for (row = 0; row < image.rows; row++)
      for (col = 0; col < image.columns; col++) {
	uint16_t *outp =
	  &image.data[image.row_stride*row + image.channels*col + 2];
	uint16_t *row1 =
	  &qtop.data[qtop.row_stride*2*row + qtop.channels*2*col];
	uint16_t *row2 =
	  &qtop.data[qtop.row_stride*(2*row+1) + qtop.channels*2*col];
	uint32_t sum =
	  row1[0] + row1[qtop.channels] + row2[0] + row2[qtop.channels];
	int32_t out = (int32_t)round(scale[2] * (sum/4.0 - black_level[2]) +
				     ilevels->black[2]);

	if (out < 0) *outp = 0;
	else if (out > 65535) *outp = 65535;
	else *outp = out;
      }

    /* Preprocess Quattro top layer (Q->top16) at full resolution */
    for (row = 0; row < qtop.rows; row++)
      for (col = 0; col < qtop.columns; col++) {
	uint16_t *valp = &qtop.data[qtop.row_stride*row + qtop.channels*col];
	int32_t out = (int32_t)round(scale[2] * (*valp - black_level[2]) +
				     ilevels->black[2]);

	if (out < 0) *valp = 0;
	else if (out > 65535) *valp = 65535;
	else *valp = out;
      }
    interpolate_bad_pixels(x3f, &qtop, 1);
  }

  interpolate_bad_pixels(x3f, &image, 3);

  return 1;
}

/* Converts the data in place */

#define LUTSIZE 1024

static int convert_data(x3f_t *x3f,
			x3f_area16_t *image, image_levels_t *ilevels,
			x3f_color_encoding_t encoding, char *wb)
{
  int row, col, color;
  uint16_t max_out = 65535; /* TODO: should be possible to adjust */

  double raw_to_xyz[9];	/* White point for XYZ is assumed to be D65 */
  double xyz_to_rgb[9];
  double raw_to_rgb[9];
  double conv_matrix[9];
  double sensor_iso, capture_iso, iso_scaling;
  double lut[LUTSIZE];
  spatial_gain_corr_t sgain[MAXCORR];
  int sgain_num;

  if (image->channels < 3) return X3F_ARGUMENT_ERROR;

  if (x3f_get_camf_float(x3f, "SensorISO", &sensor_iso) &&
      x3f_get_camf_float(x3f, "CaptureISO", &capture_iso)) {
    printf("SensorISO = %g\n", sensor_iso);
    printf("CaptureISO = %g\n", capture_iso);
    iso_scaling = capture_iso/sensor_iso;
  }
  else {
    iso_scaling = 1.0;
    fprintf(stderr, "WARNING: could not calculate ISO scaling, assuming %g\n",
	    iso_scaling);
  }

  if (!get_raw_to_xyz(x3f, wb, raw_to_xyz)) {
    fprintf(stderr, "Could not get raw_to_xyz for white balance: %s\n", wb);
    return 0;
  }

  switch (encoding) {
  case SRGB:
    x3f_sRGB_LUT(lut, LUTSIZE, max_out);
    x3f_XYZ_to_sRGB(xyz_to_rgb);
    break;
  case ARGB:
    x3f_gamma_LUT(lut, LUTSIZE, max_out, 2.2);
    x3f_XYZ_to_AdobeRGB(xyz_to_rgb);
    break;
  case PPRGB:
    {
      double xyz_to_prophotorgb[9], d65_to_d50[9];

      x3f_gamma_LUT(lut, LUTSIZE, max_out, 1.8);
      x3f_XYZ_to_ProPhotoRGB(xyz_to_prophotorgb);
      /* The standad white point for ProPhoto RGB is D50 */
      x3f_Bradford_D65_to_D50(d65_to_d50);
      x3f_3x3_3x3_mul(xyz_to_prophotorgb, d65_to_d50, xyz_to_rgb);
    }
    break;
  default:
    fprintf(stderr, "Unknown color space %d\n", encoding);
    return X3F_ARGUMENT_ERROR;
  }

  x3f_3x3_3x3_mul(xyz_to_rgb, raw_to_xyz, raw_to_rgb);
  x3f_scalar_3x3_mul(iso_scaling, raw_to_rgb, conv_matrix);

  printf("raw_to_rgb\n");
  x3f_3x3_print(raw_to_rgb);
  printf("conv_matrix\n");
  x3f_3x3_print(conv_matrix);

  sgain_num = get_spatial_gain(x3f, wb, sgain);
  if (sgain_num == 0)
    fprintf(stderr, "WARNING: could not get spatial gain\n");

  for (row = 0; row < image->rows; row++) {
    for (col = 0; col < image->columns; col++) {
      uint16_t *valp[3];
      double input[3], output[3];

      /* Get the data */
      for (color = 0; color < 3; color++) {
	valp[color] =
	  &image->data[image->row_stride*row + image->channels*col + color];
	input[color] = calc_spatial_gain(sgain, sgain_num,
					 row, col, color,
					 image->rows, image->columns) *
	  (*valp[color] - ilevels->black[color]) /
	  (ilevels->white[color] - ilevels->black[color]);
      }

      /* Do color conversion */
      x3f_3x3_3x1_mul(conv_matrix, input, output);

      /* Write back the data, doing non linear coding */
      for (color = 0; color < 3; color++)
	*valp[color] = x3f_LUT_lookup(lut, LUTSIZE, output[color]);
    }
  }

  cleanup_spatial_gain(sgain, sgain_num);

  ilevels->black[0] = ilevels->black[1] = ilevels->black[2] = 0.0;
  ilevels->white[0] = ilevels->white[1] = ilevels->white[2] = max_out;

  return 1;
}

static int run_denoising(x3f_t *x3f)
{
  x3f_area16_t original_image, image;
  x3f_denoise_type_t type = X3F_DENOISE_STD;
  char *sensorid;

  if (!x3f_image_area(x3f, &original_image)) return 0;
  if (!x3f_crop_area_camf(x3f, "ActiveImageArea", &original_image, 1, &image)) {
    image = original_image;
    fprintf(stderr,
	    "WARNING: could not get active area, denoising entire image\n");
  }

  if (x3f_get_prop_entry(x3f, "SENSORID", &sensorid) &&
      !strcmp(sensorid, "F20"))
    type = X3F_DENOISE_F20;

  x3f_denoise(&image, type);
  return 1;
}

static int expand_quattro(x3f_t *x3f, int denoise, x3f_area16_t *expanded)
{
  x3f_area16_t image, active, qtop, qtop_crop, active_exp;
  uint32_t rect[4];

  if (!x3f_image_area_qtop(x3f, &qtop)) return 0;
  if (!x3f_image_area(x3f, &image)) return 0;
  if (denoise &&
      !x3f_crop_area_camf(x3f, "ActiveImageArea", &image, 1, &active)) {
    active = image;
    fprintf(stderr,
	    "WARNING: could not get active area, denoising entire image\n");
  }

  rect[0] = 0;
  rect[1] = 0;
  rect[2] = 2*image.columns - 1;
  rect[3] = 2*image.rows - 1;
  if (!x3f_crop_area(rect, &qtop, &qtop_crop)) return 0;

  expanded->columns = qtop_crop.columns;
  expanded->rows = qtop_crop.rows;
  expanded->channels = 3;
  expanded->row_stride = expanded->columns*expanded->channels;
  expanded->data = expanded->buf =
    malloc(expanded->rows*expanded->row_stride*sizeof(uint16_t));

  if (denoise && !x3f_crop_area_camf(x3f, "ActiveImageArea", expanded, 0,
				     &active_exp)) {
    active_exp = *expanded;
    fprintf(stderr,
	    "WARNING: could not get active area, denoising entire image\n");
  }

  x3f_expand_quattro(&image, denoise ? &active : NULL, &qtop_crop,
		     expanded, denoise ? &active_exp : NULL);

  return 1;
}

static int get_image(x3f_t *x3f,
		     x3f_area16_t *image,
		     image_levels_t *ilevels,
		     x3f_color_encoding_t encoding,
		     int crop,
		     int denoise,
		     char *wb)
{
  x3f_area16_t original_image, expanded;
  image_levels_t il;

  if (wb == NULL) wb = x3f_get_wb(x3f);

  if (encoding == QTOP) {
    x3f_area16_t qtop;

    if (!x3f_image_area_qtop(x3f, &qtop)) return 0;
    if (!crop || !x3f_crop_area_camf(x3f, "ActiveImageArea", &qtop, 0, image))
      *image = qtop;

    return ilevels == NULL;
  }

  if (!x3f_image_area(x3f, &original_image)) return 0;
  if (!crop || !x3f_crop_area_camf(x3f, "ActiveImageArea", &original_image, 1,
				   image))
    *image = original_image;

  if (encoding == UNPROCESSED) return ilevels == NULL;

  if (!preprocess_data(x3f, wb, &il)) return 0;

  if (expand_quattro(x3f, denoise, &expanded)) {
    /* NOTE: expand_quattro destroys the data of original_image */
    if (!crop ||
	!x3f_crop_area_camf(x3f, "ActiveImageArea", &expanded, 0, image))
      *image = expanded;
    original_image = expanded;
  }
  else if (denoise && !run_denoising(x3f)) return 0;

  if (encoding != NONE &&
      !convert_data(x3f, &original_image, &il, encoding, wb)) {
    free(image->buf);
    return 0;
  }

  if (ilevels) *ilevels = il;
  return 1;
}

/* Routines for writing big endian ppm data */

static void write_16B(FILE *f_out, uint16_t val)
{
  fputc((val>>8) & 0xff, f_out);
  fputc((val>>0) & 0xff, f_out);
}

/* extern */
x3f_return_t x3f_dump_raw_data_as_ppm(x3f_t *x3f,
				      char *outfilename,
				      x3f_color_encoding_t encoding,
				      int crop,
				      int denoise,
				      char *wb,
				      int binary)
{
  x3f_area16_t image;
  FILE *f_out = fopen(outfilename, "wb");
  int row;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  if (!get_image(x3f, &image, NULL, encoding, crop, denoise, wb) ||
      image.channels < 3) {
    fclose(f_out);
    return X3F_ARGUMENT_ERROR;
  }

  if (binary)
    fprintf(f_out, "P6\n%d %d\n65535\n", image.columns, image.rows);
  else
    fprintf(f_out, "P3\n%d %d\n65535\n", image.columns, image.rows);

  for (row=0; row < image.rows; row++) {
    int col;

    for (col=0; col < image.columns; col++) {
      int color;

      for (color=0; color < 3; color++) {
	uint16_t val = image.data[image.row_stride*row + image.channels*col + color];
	if (binary)
	  write_16B(f_out, val);
	else
	  fprintf(f_out, "%d ", val);
      }
      if (!binary)
	fprintf(f_out, "\n");
    }
  }

  fclose(f_out);
  free(image.buf);

  return X3F_OK;
}

/* extern */
x3f_return_t x3f_dump_raw_data_as_tiff(x3f_t *x3f,
				       char *outfilename,
				       x3f_color_encoding_t encoding,
				       int crop,
				       int denoise,
				       char *wb)
{
  x3f_area16_t image;
  TIFF *f_out = TIFFOpen(outfilename, "w");
  int row;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  if (!get_image(x3f, &image, NULL, encoding, crop, denoise, wb)) {
    TIFFClose(f_out);
    return X3F_ARGUMENT_ERROR;
  }

  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, image.columns);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, image.rows);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, 32);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, image.channels);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, image.channels == 1 ?
	       PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
  TIFFSetField(f_out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(f_out, TIFFTAG_XRESOLUTION, 72.0);
  TIFFSetField(f_out, TIFFTAG_YRESOLUTION, 72.0);
  TIFFSetField(f_out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

  for (row=0; row < image.rows; row++)
    TIFFWriteScanline(f_out, image.data + image.row_stride*row, row, 0);

  TIFFWriteDirectory(f_out);
  TIFFClose(f_out);
  free(image.buf);

  return X3F_OK;
}

static void vec_double_to_float(double *a, float *b, int len)
{
  int i;

  for (i=0; i<len; i++)
    b[i] = a[i];
}

static int get_camf_rect_as_dngrect(x3f_t *x3f, char *name,
				    x3f_area16_t *image, int rescale,
				    uint32_t *rect)
{
  uint32_t camf_rect[4];

  if (!x3f_get_camf_rect(x3f, name, image, rescale, camf_rect))
    return 0;

  /* Translate from Sigma's to Adobe's view on rectangles */
  rect[0] = camf_rect[1];
  rect[1] = camf_rect[0];
  rect[2] = camf_rect[3] + 1;
  rect[3] = camf_rect[2] + 1;

  return 1;
}

static int write_spatial_gain(x3f_t *x3f, x3f_area16_t *image, char *wb,
			      TIFF *tiff)
{
  spatial_gain_corr_t corr[MAXCORR];
  int corr_num;
  uint32_t active_area[4];
  double originv, originh, scalev, scaleh;

  uint8_t *opcode_list, *p;
  int opcode_size[MAXCORR];
  int opcode_list_size = sizeof(dng_opcodelist_header_t);
  dng_opcodelist_header_t *header;
  int i, j;

  if (!get_camf_rect_as_dngrect(x3f, "ActiveImageArea", image, 1,
				active_area))
    return 0;

  /* Spatial gain in X3F refers to the entire image, but OpcodeList2
     in DNG is appled after cropping to ActiveArea */
  originv = -(double)active_area[0] / (active_area[2] - active_area[0]);
  originh = -(double)active_area[1] / (active_area[3] - active_area[1]);
  scalev =   (double)image->rows    / (active_area[2] - active_area[0]);
  scaleh =   (double)image->columns / (active_area[3] - active_area[1]);

  corr_num = get_spatial_gain(x3f, wb, corr);
  if (corr_num == 0) return 0;

  for (i=0; i<corr_num; i++) {
    opcode_size[i] = sizeof(dng_opcode_GainMap_t) +
      corr[i].rows*corr[i].cols*corr[i].channels*sizeof(float);
    opcode_list_size += opcode_size[i];
  }

  opcode_list = alloca(opcode_list_size);
  header = (dng_opcodelist_header_t *)opcode_list;
  PUT_BIG_32(header->count, corr_num);

  for (p = opcode_list + sizeof(dng_opcodelist_header_t), i=0;
       i<corr_num;
       p += opcode_size[i], i++) {
    dng_opcode_GainMap_t *gain_map = (dng_opcode_GainMap_t *)p;
    spatial_gain_corr_t *c = &corr[i];

    PUT_BIG_32(gain_map->header.id, DNG_OPCODE_GAINMAP_ID);
    PUT_BIG_32(gain_map->header.ver, DNG_OPCODE_GAINMAP_VER);
    PUT_BIG_32(gain_map->header.flags, 0);
    PUT_BIG_32(gain_map->header.parsize,
	       opcode_size[i] - sizeof(dng_opcode_header_t));

    PUT_BIG_32(gain_map->Top, c->rowoff);
    PUT_BIG_32(gain_map->Left, c->coloff);
    PUT_BIG_32(gain_map->Bottom, active_area[2] - active_area[0]);
    PUT_BIG_32(gain_map->Right, active_area[3] - active_area[1]);
    PUT_BIG_32(gain_map->Plane, c->chan);
    PUT_BIG_32(gain_map->Planes, c->channels);
    PUT_BIG_32(gain_map->RowPitch, c->rowpitch);
    PUT_BIG_32(gain_map->ColPitch, c->colpitch);
    PUT_BIG_32(gain_map->MapPointsV, c->rows);
    PUT_BIG_32(gain_map->MapPointsH, c->cols);
    PUT_BIG_64(gain_map->MapSpacingV, scalev/(c->rows-1));
    PUT_BIG_64(gain_map->MapSpacingH, scaleh/(c->cols-1));
    PUT_BIG_64(gain_map->MapOriginV, originv);
    PUT_BIG_64(gain_map->MapOriginH, originh);
    PUT_BIG_32(gain_map->MapPlanes, c->channels);

    for (j=0; j<c->rows*c->cols*c->channels; j++)
      PUT_BIG_32(gain_map->MapGain[j], c->gain[j]);
  }

  cleanup_spatial_gain(corr, corr_num);

  TIFFSetField(tiff, TIFFTAG_OPCODELIST2, opcode_list_size, opcode_list);

  return 1;
}

typedef struct {
  char *name;
  int (*get_raw_to_xyz)(x3f_t *x3f, char *wb, double *raw_to_xyz);
} camera_profile_t;

static const camera_profile_t camera_profiles[] = {
  {"Default", get_raw_to_xyz},
  {"Unconverted", get_raw_to_xyz_noconvert},
};

static int write_camera_profile(x3f_t *x3f, char *wb,
				const camera_profile_t *profile,
				TIFF *tiff)
{
  double raw_to_xyz[9], xyz_to_raw[9];
  float color_matrix1[9];

  if (!profile->get_raw_to_xyz(x3f, wb, raw_to_xyz)) {
    fprintf(stderr, "Could not get raw_to_xyz for white balance: %s\n", wb);
    return 0;
  }
  x3f_3x3_inverse(raw_to_xyz, xyz_to_raw);
  vec_double_to_float(xyz_to_raw, color_matrix1, 9);
  TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, color_matrix1);

  TIFFSetField(tiff, TIFFTAG_PROFILENAME, profile->name);
  /* Tell the raw converter to refrain from clipping the dark areas */
  TIFFSetField(tiff, TIFFTAG_DEFAULTBLACKRENDER, 1);

  return 1;
}

static x3f_return_t write_camera_profiles(x3f_t *x3f, char *wb,
					  const camera_profile_t *profiles,
					  int num,
					  TIFF *tiff)
{
  FILE *tiff_file;
  uint32_t *profile_offsets;
  int i;

  assert(num >= 1);
  if (!write_camera_profile(x3f, wb, &profiles[0], tiff))
    return X3F_ARGUMENT_ERROR;
  TIFFSetField(tiff, TIFFTAG_ASSHOTPROFILENAME, profiles[0].name);
  if (num == 1) return X3F_OK;

  profile_offsets = alloca((num-1)*sizeof(uint32_t));

  tiff_file = fdopen(dup(TIFFFileno(tiff)), "w+");
  if (!tiff_file) return X3F_OUTFILE_ERROR;

  for (i=1; i < num; i++) {
    FILE *tmp;
    TIFF *tmp_tiff;
#define BUFSIZE 1024
    char buf[BUFSIZE];
    int offset, count;

    if (!(tmp = tmpfile())) {
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!(tmp_tiff = TIFFFdOpen(dup(fileno(tmp)), "", "wb"))) { /* Big endian */
      fclose(tmp);
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!write_camera_profile(x3f, wb, &profiles[i], tmp_tiff)) {
      fclose(tmp);
      TIFFClose(tmp_tiff);
      fclose(tiff_file);
      return X3F_ARGUMENT_ERROR;
    }
    TIFFWriteDirectory(tmp_tiff);
    TIFFClose(tmp_tiff);

    fseek(tiff_file, 0, SEEK_END);
    offset = (ftell(tiff_file)+1) & ~1; /* 2-byte alignment */
    fseek(tiff_file, offset, SEEK_SET);
    profile_offsets[i-1] = offset;

    fputs("MMCR", tiff_file);	/* DNG camera profile magic in big endian */
    fseek(tmp, 4, SEEK_SET);	/* Skip over the standard TIFF magic */

    while((count = fread(buf, 1, BUFSIZE, tmp)))
      fwrite(buf, 1, count, tiff_file);

    fclose(tmp);
  }

  fclose(tiff_file);
  TIFFSetField(tiff, TIFFTAG_EXTRACAMERAPROFILES, num-1, profile_offsets);
  return X3F_OK;
}

/* extern */
x3f_return_t x3f_dump_raw_data_as_dng(x3f_t *x3f,
				      char *outfilename,
				      int denoise,
				      char *wb)
{
  x3f_return_t ret;
  TIFF *f_out;
  uint64_t sub_ifds[1] = {0};
#define THUMBSIZE 100
  uint8_t thumbnail[3*THUMBSIZE];

  double sensor_iso, capture_iso;
  double gain[3], gain_inv[3];
  float as_shot_neutral[3];
  float black_level[3];
  uint32_t active_area[4];
  x3f_area16_t image;
  image_levels_t ilevels;
  int row;

  if (wb == NULL) wb = x3f_get_wb(x3f);
  if (!get_image(x3f, &image, &ilevels, NONE, 0, denoise, wb) ||
      image.channels != 3)
    return X3F_ARGUMENT_ERROR;

  x3f_dngtags_install_extender();

  f_out = TIFFOpen(outfilename, "w");
  if (f_out == NULL) {
    free(image.buf);
    return X3F_OUTFILE_ERROR;
  }

  TIFFSetField(f_out, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, THUMBSIZE);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, THUMBSIZE);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, THUMBSIZE);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, 3);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(f_out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(f_out, TIFFTAG_DNGVERSION, "\001\004\000\000");
  TIFFSetField(f_out, TIFFTAG_DNGBACKWARDVERSION, "\001\004\000\000");
  TIFFSetField(f_out, TIFFTAG_SUBIFD, 1, sub_ifds);
  /* Prevent clipping of dark areas in DNG processing software */
  TIFFSetField(f_out, TIFFTAG_DEFAULTBLACKRENDER, 1);

  if (x3f_get_camf_float(x3f, "SensorISO", &sensor_iso) &&
      x3f_get_camf_float(x3f, "CaptureISO", &capture_iso)) {
    double baseline_exposure = log2(capture_iso/sensor_iso);
    TIFFSetField(f_out, TIFFTAG_BASELINEEXPOSURE, baseline_exposure);
  }

  ret = write_camera_profiles(x3f, wb, camera_profiles,
			      sizeof(camera_profiles)/sizeof(camera_profile_t),
			      f_out);
  if (ret != X3F_OK) {
    fprintf(stderr, "Could not write camera profiles\n");
    TIFFClose(f_out);
    free(image.buf);
    return ret;
  }

  if (!get_gain(x3f, wb, gain)) {
    fprintf(stderr, "Could not get gain for white balance: %s\n", wb);
    TIFFClose(f_out);
    free(image.buf);
    return X3F_ARGUMENT_ERROR;
  }
  x3f_3x1_invert(gain, gain_inv);
  vec_double_to_float(gain_inv, as_shot_neutral, 3);
  TIFFSetField(f_out, TIFFTAG_ASSHOTNEUTRAL, 3, as_shot_neutral);

  memset(thumbnail, 0, 3*THUMBSIZE); /* TODO: Thumbnail is all black */
  for (row=0; row < THUMBSIZE; row++)
    TIFFWriteScanline(f_out, thumbnail, row, 0);

  TIFFWriteDirectory(f_out);

  TIFFSetField(f_out, TIFFTAG_SUBFILETYPE, 0);
  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, image.columns);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, image.rows);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, 32);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, 3);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LINEARRAW);
  /* Prevent further chroma denoising in DNG processing software */
  TIFFSetField(f_out, TIFFTAG_CHROMABLURRADIUS, 0.0);

  vec_double_to_float(ilevels.black, black_level, 3);
  TIFFSetField(f_out, TIFFTAG_BLACKLEVEL, 3, black_level);
  TIFFSetField(f_out, TIFFTAG_WHITELEVEL, 3, ilevels.white);

  if (!write_spatial_gain(x3f, &image, wb, f_out))
    fprintf(stderr, "WARNING: could not get spatial gain\n");

  if (get_camf_rect_as_dngrect(x3f, "ActiveImageArea", &image, 1, active_area))
    TIFFSetField(f_out, TIFFTAG_ACTIVEAREA, active_area);

  for (row=0; row < image.rows; row++)
    TIFFWriteScanline(f_out, image.data + image.row_stride*row, row, 0);

  TIFFWriteDirectory(f_out);
  TIFFClose(f_out);
  free(image.buf);

  return X3F_OK;
}

static int ilog(int i, double base, int steps)
{
  if (i <= 0)
    /* Special case as log(0) is not defined. */
    return 0;
  else {
    double log = log10((double)i) / log10(base);

    return (int)(steps * log);
  }
}

static int ilog_inv(int i, double base, int steps)
{
  return (int)round(pow(base, (double)i/steps));
}

#define BASE 2.0
#define STEPS 10

/* extern */
x3f_return_t x3f_dump_raw_data_as_histogram(x3f_t *x3f,
					    char *outfilename,
					    x3f_color_encoding_t encoding,
					    int crop,
					    int denoise,
					    char *wb,
					    int log_hist)
{
  x3f_area16_t image;
  FILE *f_out = fopen(outfilename, "wb");
  uint32_t *histogram[3];
  int color, i;
  int row;
  uint16_t max = 0;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  if (!get_image(x3f, &image, NULL, encoding, crop, denoise, wb) ||
      image.channels < 3) {
    fclose(f_out);
    return X3F_ARGUMENT_ERROR;
  }

  for (color=0; color < 3; color++)
    histogram[color] = (uint32_t *)calloc(1<<16, sizeof(uint32_t));

  for (row=0; row < image.rows; row++) {
    int col;

    for (col=0; col < image.columns; col++)
      for (color=0; color < 3; color++) {
	uint16_t val =
	  image.data[image.row_stride*row + image.channels*col + color];

	if (log_hist)
	  val = ilog(val, BASE, STEPS);

	histogram[color][val]++;
	if (val > max) max = val;
      }
  }

  for (i=0; i <= max; i++) {
    uint32_t val[3];

    for (color=0; color < 3; color++)
      val[color] = histogram[color][i];

    if (val[0] || val[1] || val[2]) {
      if (log_hist) {
	fprintf(f_out, "%5d, %5d , %6d , %6d , %6d\n",
		i, ilog_inv(i, BASE, STEPS), val[0], val[1], val[2]);
      } else {
	fprintf(f_out, "%5d , %6d , %6d , %6d\n",
		i, val[0], val[1], val[2]);
      }
    }
  }

  for (color=0; color < 3; color++)
    free(histogram[color]);

  fclose(f_out);
  free(image.buf);

  return X3F_OK;
}
