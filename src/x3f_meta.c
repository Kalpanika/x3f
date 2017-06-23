/* X3F_META.C
 *
 * Library for accessing the CAMF and PROP meta data in the X3F file.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_meta.h"
#include "x3f_io.h"
#include "x3f_printf.h"

#include <stdio.h>
#include <string.h>

/* extern */ int x3f_get_camf_text(x3f_t *x3f, char *name, char **text)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    x3f_printf(DEBUG, "Could not get entry %s: CAMF section not found\n", name);
    return 0;
  }

  DEH = &DE->header;
  CAMF = &DEH->data_subsection.camf;
  table = CAMF->entry_table.element;

  for (i=0; i<CAMF->entry_table.size; i++) {
    camf_entry_t *entry = &table[i];

    if (!strcmp(name, entry->name_address)) {
      if (entry->id != X3F_CMbT) {
	x3f_printf(DEBUG, "CAMF entry is not text: %s\n", name);
	return 0;
      }

      *text = entry->text;
      return 1;
    }
  }

  x3f_printf(DEBUG, "CAMF entry not found: %s\n", name);

  return 0;
}

/* extern */ int x3f_get_camf_matrix_var(x3f_t *x3f, char *name,
					 int *dim0, int *dim1, int *dim2,
					 matrix_type_t type,
					 void **matrix)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    x3f_printf(DEBUG, "Could not get entry %s: CAMF section not found\n", name);
    return 0;
  }

  DEH = &DE->header;
  CAMF = &DEH->data_subsection.camf;
  table = CAMF->entry_table.element;

  for (i=0; i<CAMF->entry_table.size; i++) {
    camf_entry_t *entry = &table[i];

    if (!strcmp(name, entry->name_address)) {
      if (entry->id != X3F_CMbM) {
	x3f_printf(DEBUG, "CAMF entry is not a matrix: %s\n", name);
	return 0;
      }
      if (entry->matrix_decoded_type != type) {
	x3f_printf(DEBUG, "CAMF entry not required type: %s\n", name);
	return 0;
      }

      switch (entry->matrix_dim) {
      case 3:
	if (dim2 == NULL || dim1 == NULL || dim0 == NULL) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim2 = entry->matrix_dim_entry[2].size;
	*dim1 = entry->matrix_dim_entry[1].size;
	*dim0 = entry->matrix_dim_entry[0].size;
      break;
      case 2:
	if (dim2 != NULL || dim1 == NULL || dim0 == NULL) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim1 = entry->matrix_dim_entry[1].size;
	*dim0 = entry->matrix_dim_entry[0].size;
      break;
      case 1:
	if (dim2 != NULL || dim1 != NULL || dim0 == NULL) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim0 = entry->matrix_dim_entry[0].size;
	break;
      default:
	x3f_printf(DEBUG, "CAMF entry - more than 3 dimensions: %s\n", name);
	return 0;
      }

      x3f_printf(DEBUG, "Getting CAMF matrix for %s\n", name);
      *matrix = entry->matrix_decoded;
      return 1;
    }
  }

  x3f_printf(DEBUG, "CAMF entry not found: %s\n", name);

  return 0;
}

/* extern */ int x3f_get_camf_matrix(x3f_t *x3f, char *name,
				     int dim0, int dim1, int dim2,
				     matrix_type_t type,
				     void *matrix)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    x3f_printf(DEBUG, "Could not get entry %s: CAMF section not found\n", name);
    return 0;
  }

  DEH = &DE->header;
  CAMF = &DEH->data_subsection.camf;
  table = CAMF->entry_table.element;

  for (i=0; i<CAMF->entry_table.size; i++) {
    camf_entry_t *entry = &table[i];

    if (!strcmp(name, entry->name_address)) {
      int size;

      if (entry->id != X3F_CMbM) {
	x3f_printf(DEBUG, "CAMF entry is not a matrix: %s\n", name);
	return 0;
      }
      if (entry->matrix_decoded_type != type) {
	x3f_printf(DEBUG, "CAMF entry not required type: %s\n", name);
	return 0;
      }

      switch (entry->matrix_dim) {
      case 3:
	if (dim2 != entry->matrix_dim_entry[2].size ||
	    dim1 != entry->matrix_dim_entry[1].size ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      case 2:
	if (dim2 != 0 ||
	    dim1 != entry->matrix_dim_entry[1].size ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      case 1:
	if (dim2 != 0 ||
	    dim1 != 0 ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  x3f_printf(DEBUG, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      default:
	x3f_printf(DEBUG, "CAMF entry - more than 3 dimensions: %s\n", name);
	return 0;
      }

      size = (entry->matrix_decoded_type==M_FLOAT ?
	      sizeof(double) :
	      sizeof(uint32_t)) * entry->matrix_elements;
      x3f_printf(DEBUG, "Copying CAMF matrix for %s\n", name);
      memcpy(matrix, entry->matrix_decoded, size);
      return 1;
    }
  }

  x3f_printf(DEBUG, "CAMF entry not found: %s\n", name);

  return 0;
}

/* extern */ int x3f_get_camf_float(x3f_t *x3f, char *name,  double *val)
{
  return x3f_get_camf_matrix(x3f, name, 1, 0, 0, M_FLOAT, val);
}

/* extern */ int x3f_get_camf_float_vector(x3f_t *x3f, char *name,  double *val)
{
  return x3f_get_camf_matrix(x3f, name, 3, 0, 0, M_FLOAT, val);
}

/* extern */ int x3f_get_camf_unsigned(x3f_t *x3f, char *name,  uint32_t *val)
{
  return x3f_get_camf_matrix(x3f, name, 1, 0, 0, M_UINT, val);
}

/* extern */ int x3f_get_camf_signed(x3f_t *x3f, char *name,  int32_t *val)
{
  return x3f_get_camf_matrix(x3f, name, 1, 0, 0, M_INT, val);
}

/* extern */ int x3f_get_camf_signed_vector(x3f_t *x3f, char *name,
					    int32_t *val)
{
  return x3f_get_camf_matrix(x3f, name, 3, 0, 0, M_INT, val);
}

/* extern */ int x3f_get_camf_property_list(x3f_t *x3f, char *list,
					    char ***names, char ***values,
					    uint32_t *num)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    x3f_printf(DEBUG, "Could not get entry %s: CAMF section not found\n",
	       list);
    return 0;
  }

  DEH = &DE->header;
  CAMF = &DEH->data_subsection.camf;
  table = CAMF->entry_table.element;

  for (i=0; i<CAMF->entry_table.size; i++) {
    camf_entry_t *entry = &table[i];

    if (!strcmp(list, entry->name_address)) {
      if (entry->id != X3F_CMbP) {
	x3f_printf(DEBUG, "CAMF entry is not a property list: %s\n", list);
	return 0;
      }

      x3f_printf(DEBUG, "Getting CAMF property list for %s\n", list);
      *names = entry->property_name;
      *values = (char **)entry->property_value;
      *num = entry->property_num;
      return 1;
    }
  }

  x3f_printf(DEBUG, "CAMF entry not found: %s\n", list);

  return 0;
}

/* extern */ int x3f_get_camf_property(x3f_t *x3f, char *list,
				       char *name, char **value)
{
  char **names, **values;
  uint32_t num, i;

  if (!x3f_get_camf_property_list(x3f, list, &names, &values, &num))
    return 0;

  for (i=0; i<num; i++)
    if (!strcmp(names[i], name)) {
	*value = values[i];
	return 1;
    }

  x3f_printf(DEBUG, "CAMF property '%s' not found in list '%s'\n", name, list);

  return 0;
}

/* extern */ int x3f_get_prop_entry(x3f_t *x3f, char *name, char **value)
{
  x3f_directory_entry_t *DE = x3f_get_prop(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_property_list_t *PL;
  x3f_property_t *table;
  int i;

  if (!DE) {
    x3f_printf(DEBUG, "Could not get property %s: PROP section not found\n",
	       name);
    return 0;
  }

  DEH = &DE->header;
  PL = &DEH->data_subsection.property_list;
  table = PL->property_table.element;

  for (i=0; i<PL->property_table.size; i++) {
    x3f_property_t *entry = &table[i];

    if (!strcmp(name, entry->name_utf8)) {
      x3f_printf(DEBUG, "Getting PROP entry \"%s\" = \"%s\"\n",
		 name, entry->value_utf8);
      *value = entry->value_utf8;
      return 1;
    }
  }

  x3f_printf(DEBUG, "PROP entry not found: %s\n", name);

  return 0;
}

/* extern */ char *x3f_get_wb(x3f_t *x3f)
{
  uint32_t wb;

  if (x3f_get_camf_unsigned(x3f, "WhiteBalance", &wb)) {
    /* Quattro. TODO: any better way to do this? Maybe get the info
       from the EXIF in the JPEG? */
    switch (wb) {
    case 1:  return "Auto";
    case 2:  return "Sunlight";
    case 3:  return "Shadow";
    case 4:  return "Overcast";
    case 5:  return "Incandescent";
    case 6:  return "Florescent";
    case 7:  return "Flash";
    case 8:  return "Custom";
    case 11: return "ColorTemp";
    case 12: return "AutoLSP";
    default: return "Auto";
    }
  }

  return x3f->header.white_balance;
}

/* extern */ int x3f_get_camf_matrix_for_wb(x3f_t *x3f,
					    char *list, char *wb,
					    int dim0, int dim1,
					    double *matrix)
{
  char *matrix_name;

  if (!x3f_get_camf_property(x3f, list, wb, &matrix_name)) {
    if (!strcmp(wb, "Daylight")) /* Workaround for bug in SD1 */
      return x3f_get_camf_matrix_for_wb(x3f, list, "Sunlight", dim0, dim1,
					matrix);
    return 0;
  }

  return x3f_get_camf_matrix(x3f, matrix_name, dim0, dim1, 0, M_FLOAT, matrix);
}

static int x3f_is_TRUE_engine(x3f_t *x3f)
{
  char **names, **values;
  uint32_t num;

  /* TODO: is there a better way to test this */
  if ((x3f_get_camf_property_list(x3f, "WhiteBalanceColorCorrections",
				  &names, &values, &num) ||
       x3f_get_camf_property_list(x3f, "DP1_WhiteBalanceColorCorrections",
				  &names, &values, &num)) &&
      (x3f_get_camf_property_list(x3f, "WhiteBalanceGains",
				  &names, &values, &num) ||
       x3f_get_camf_property_list(x3f, "DP1_WhiteBalanceGains",
				  &names, &values, &num)))
    return 1;
  else
    return 0;
}

/*
  Candidates for getting Max RAW level:
  - SaturationLevel: x530, SD9, SD10, SD14, SD15, DP1-DP2x
  - RawSaturationLevel:               SD14, SD15, DP1-DP2x
  - MaxOutputLevel:                   SD14, SD15, DP1-DP2x
  - ImageDepth: Merrill and Quattro
*/

/* extern */ int x3f_get_max_raw(x3f_t *x3f, uint32_t *max_raw)
{
  uint32_t image_depth;

  if (x3f_get_camf_unsigned(x3f, "ImageDepth", &image_depth)) {
    uint32_t max = (1<<image_depth) - 1;
    max_raw[0] = max;
    max_raw[1] = max;
    max_raw[2] = max;
    return 1;
  }

  /* It seems that RawSaturationLevel should be used for TURE engine and
     SaturationLevel for pre-TRUE engine */
  if (x3f_is_TRUE_engine(x3f))
    return x3f_get_camf_signed_vector(x3f, "RawSaturationLevel",
				      (int32_t *)max_raw);
  else
    return x3f_get_camf_signed_vector(x3f, "SaturationLevel",
				      (int32_t *)max_raw);
}
