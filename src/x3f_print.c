/* X3F_PRINT.C - Library for accessing X3F Files.
 *
 * Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_print.h"
#include "x3f_io.h"

#include <stdio.h>

/* --------------------------------------------------------------------- */
/* Hacky external flags                                                 */
/* --------------------------------------------------------------------- */

/* extern */ uint32_t max_printed_matrix_elements = 100;

/* --------------------------------------------------------------------- */
/* Pretty print the x3f structure                                        */
/* --------------------------------------------------------------------- */

static char x3f_id_buf[5] = {0,0,0,0,0};

static char *x3f_id(uint32_t id)
{
  x3f_id_buf[0] = (id>>0) & 0xff;
  x3f_id_buf[1] = (id>>8) & 0xff;
  x3f_id_buf[2] = (id>>16) & 0xff;
  x3f_id_buf[3] = (id>>24) & 0xff;

  return x3f_id_buf;
}

static char *id_to_str(uint32_t id)
{
  char *idp = (char *)&id;
  static char buf[5];

  buf[0] = *(idp + 0);
  buf[1] = *(idp + 1);
  buf[2] = *(idp + 2);
  buf[3] = *(idp + 3);
  buf[4] = 0;

  return buf;
}

static void print_matrix_element(FILE *f_out, camf_entry_t *entry, uint32_t i)
{
  switch (entry->matrix_decoded_type) {
  case M_FLOAT:
    fprintf(f_out, "%12g ", ((double *)(entry->matrix_decoded))[i]);
    break;
  case M_INT:
    fprintf(f_out, "%12d ", ((int32_t *)(entry->matrix_decoded))[i]);
    break;
  case M_UINT:
    fprintf(f_out, "%12d ", ((uint32_t *)(entry->matrix_decoded))[i]);
    break;
  }
}

static void print_matrix(FILE *f_out, camf_entry_t *entry)
{
  uint32_t dim = entry->matrix_dim;
  uint32_t linesize = entry->matrix_dim_entry[dim-1].size;
  uint32_t blocksize = (uint32_t)(-1);
  uint32_t totalsize = entry->matrix_elements;
  int i;

  switch (entry->matrix_decoded_type) {
  case M_FLOAT:
    fprintf(f_out, "float ");
    break;
  case M_INT:
    fprintf(f_out, "integer ");
    break;
  case M_UINT:
    fprintf(f_out, "unsigned integer ");
    break;
  }

  switch (dim) {
  case 1:
    fprintf(f_out, "[%d]\n", entry->matrix_dim_entry[0].size);
    fprintf(f_out, "x: %s\n", entry->matrix_dim_entry[0].name);
    break;
  case 2:
    fprintf(f_out, "[%d][%d]\n",
	    entry->matrix_dim_entry[0].size,
	    entry->matrix_dim_entry[1].size);
    fprintf(f_out, "x: %s\n", entry->matrix_dim_entry[1].name);
    fprintf(f_out, "y: %s\n", entry->matrix_dim_entry[0].name);
    break;
  case 3:
    fprintf(f_out, "[%d][%d][%d]\n",
	    entry->matrix_dim_entry[0].size,
	    entry->matrix_dim_entry[1].size,
	    entry->matrix_dim_entry[2].size);
    fprintf(f_out, "x: %s\n", entry->matrix_dim_entry[2].name);
    fprintf(f_out, "y: %s\n", entry->matrix_dim_entry[1].name);
    fprintf(f_out, "z: %s (i.e. group)\n", entry->matrix_dim_entry[0].name);
    blocksize = linesize * entry->matrix_dim_entry[dim-2].size;
    break;
  default:
    fprintf(f_out, "\nNot support for higher than 3D in printout\n");
    fprintf(stderr, "Not support for higher than 3D in printout\n");
  }

  for (i=0; i<totalsize; i++) {
    print_matrix_element(f_out, entry, i);
    if ((i+1)%linesize == 0) fprintf(f_out, "\n");
    if ((i+1)%blocksize == 0) fprintf(f_out, "\n");
    if (i >= (max_printed_matrix_elements-1)) {
      fprintf(f_out, "\n... (%d skipped) ...\n", totalsize-i-1);
      break;
    }
  }
}

static void print_file_header_meta_data(FILE *f_out, x3f_t *x3f)
{
  x3f_header_t *H = NULL;

  fprintf(f_out, "BEGIN: file header meta data\n\n");

  H = &x3f->header;
  fprintf(f_out, "header.\n");
  fprintf(f_out, "  identifier        = %08x (%s)\n", H->identifier, x3f_id(H->identifier));
  fprintf(f_out, "  version           = %08x\n", H->version);
  fprintf(f_out, "  unique_identifier = %02x...\n", H->unique_identifier[0]);
  fprintf(f_out, "  mark_bits         = %08x\n", H->mark_bits);
  fprintf(f_out, "  columns           = %08x (%d)\n", H->columns, H->columns);
  fprintf(f_out, "  rows              = %08x (%d)\n", H->rows, H->rows);
  fprintf(f_out, "  rotation          = %08x (%d)\n", H->rotation, H->rotation);
  if (x3f->header.version > X3F_VERSION_2_0) {
    int i;

    fprintf(f_out, "  white_balance     = %s\n", H->white_balance);

    fprintf(f_out, "  extended_types\n");
    for (i=0; i<NUM_EXT_DATA; i++) {
      uint8_t type = H->extended_types[i];
      float data = H->extended_data[i];

      fprintf(f_out, "    %2d: %3d = %9f\n", i, type, data);
    }
  }

  fprintf(f_out, "END: file header meta data\n\n");
}

static void print_camf_meta_data2(FILE *f_out, x3f_camf_t *CAMF)
{
  fprintf(f_out, "BEGIN: CAMF meta data\n\n");

  if (CAMF->entry_table.size != 0) {
    camf_entry_t *entry = CAMF->entry_table.element;
    int i;

    for (i=0; i<CAMF->entry_table.size; i++) {
      if (f_out == stdout) {
	fprintf(f_out, "          element[%d].name = \"%s\"\n",
		i, entry[i].name_address);
	fprintf(f_out, "            id = %x (%s)\n",
		entry[i].id, id_to_str(entry[i].id));
	fprintf(f_out, "            entry_size = %d\n",
		entry[i].entry_size);
	fprintf(f_out, "            name_size = %d\n",
		entry[i].name_size);
	fprintf(f_out, "            value_size = %d\n",
		entry[i].value_size);
      }

      /* Text CAMF */
      if (entry[i].text_size != 0) {
	fprintf(f_out, "BEGIN: CAMF text meta data (%s)\n",
		entry[i].name_address);
	fprintf(f_out, "\"%s\"\n", entry[i].text);
	fprintf(f_out, "END: CAMF text meta data\n\n");
      }

      /* Property CAMF */
      if (entry[i].property_num != 0) {
	int j;
	fprintf(f_out, "BEGIN: CAMF property meta data (%s)\n",
		entry[i].name_address);

	for (j=0; j<entry[i].property_num; j++) {
	  fprintf(f_out, "              \"%s\" = \"%s\"\n",
		  entry[i].property_name[j],
		  entry[i].property_value[j]);
	}

	fprintf(f_out, "END: CAMF property meta data\n\n");
      }

      /* Matrix CAMF */
      if (entry[i].matrix_dim != 0) {
	int j;
	camf_dim_entry_t *dentry = entry[i].matrix_dim_entry;

	fprintf(f_out, "BEGIN: CAMF matrix meta data (%s)\n",
		entry[i].name_address);

	if (f_out == stdout) {
	  fprintf(f_out, "            matrix_type = %d\n", entry[i].matrix_type);
	  fprintf(f_out, "            matrix_dim = %d\n", entry[i].matrix_dim);
	  fprintf(f_out, "            matrix_data_off = %d\n", entry[i].matrix_data_off);

	  for (j=0; j<entry[i].matrix_dim; j++) {
	    fprintf(f_out, "            %d\n", j);
	    fprintf(f_out, "              size = %d\n", dentry[j].size);
	    fprintf(f_out, "              name_offset = %d\n", dentry[j].name_offset);
	    fprintf(f_out, "              n = %d%s\n", dentry[j].n, j==dentry[j].n ? "" : " (out of order)");
	    fprintf(f_out, "              name = \"%s\"\n", dentry[j].name);
	  }

	  fprintf(f_out, "            matrix_element_size = %d\n", entry[i].matrix_element_size);
	  fprintf(f_out, "            matrix_elements = %d\n", entry[i].matrix_elements);
	  fprintf(f_out, "            matrix_estimated_element_size = %g\n", entry[i].matrix_estimated_element_size);
	}

	print_matrix(f_out, &entry[i]);

	fprintf(f_out, "END: CAMF matrix meta data\n\n");
      }
    }
  }

  fprintf(f_out, "END: CAMF meta data\n\n");
}

static void print_camf_meta_data(FILE *f_out, x3f_t *x3f)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);

  if (DE == NULL) {
    fprintf(f_out, "INFO: No CAMF meta data found\n\n");
    return;
  }

  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_camf_t *CAMF = &DEH->data_subsection.camf;

  print_camf_meta_data2(f_out, CAMF);
}

static void print_prop_meta_data2(FILE *f_out, x3f_property_list_t *PL)
{
  fprintf(f_out, "BEGIN: PROP meta data\n\n");

  if (PL->property_table.size != 0) {
    int i;
    x3f_property_t *P = PL->property_table.element;

    for (i=0; i<PL->num_properties; i++)
      fprintf(f_out, "          [%d] \"%s\" = \"%s\"\n",
	      i, P[i].name_utf8, P[i].value_utf8);
  }

  fprintf(f_out, "END: PROP meta data\n\n");
}

static void print_prop_meta_data(FILE *f_out, x3f_t *x3f)
{
  x3f_directory_entry_t *DE = x3f_get_prop(x3f);

  if (DE == NULL) {
    fprintf(f_out, "INFO: No PROP meta data found\n\n");
    return;
  }

  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_property_list_t *PL = &DEH->data_subsection.property_list;

  print_prop_meta_data2(f_out, PL);
}

/* extern */ void x3f_print(x3f_t *x3f)
{
  int d;
  x3f_directory_section_t *DS = NULL;
  x3f_info_t *I = NULL;

  if (x3f == NULL) {
    printf("Null x3f\n");
    return;
  }

  I = &x3f->info;
  printf("info.\n");
  printf("  error = %s\n", I->error);
  printf("  input.\n");
  printf("    file = %p\n", I->input.file);
  printf("  output.\n");
  printf("    file = %p\n", I->output.file);

  print_file_header_meta_data(stdout, x3f);

  DS = &x3f->directory_section;
  printf("directory_section.\n");
  printf("  identifier            = %08x (%s)\n", DS->identifier, x3f_id(DS->identifier));
  printf("  version               = %08x\n", DS->version);
  printf("  num_directory_entries = %08x\n", DS->num_directory_entries);
  printf("  directory_entry       = %p\n", DS->directory_entry);

  for (d=0; d<DS->num_directory_entries; d++) {
    x3f_directory_entry_t *DE = &DS->directory_entry[d];
    x3f_directory_entry_header_t *DEH = &DE->header;

    printf("  directory_entry.\n");
    printf("    input.\n");
    printf("      offset = %08x\n", DE->input.offset);
    printf("      size   = %08x\n", DE->input.size);
    printf("    output.\n");
    printf("      offset = %08x\n", DE->output.offset);
    printf("      size   = %08x\n", DE->output.size);
    printf("    type     = %08x (%s)\n", DE->type, x3f_id(DE->type));
    DEH = &DE->header;
    printf("    header.\n");
    printf("      identifier = %08x (%s)\n", DEH->identifier, x3f_id(DEH->identifier));
    printf("      version    = %08x\n", DEH->version);

    /* NOTE - the tests below could be made on DE->type instead */

    if (DEH->identifier == X3F_SECp) {
      x3f_property_list_t *PL = &DEH->data_subsection.property_list;
      printf("      data_subsection.property_list.\n");
      printf("        num_properties   = %08x\n", PL->num_properties);
      printf("        character_format = %08x\n", PL->character_format);
      printf("        reserved         = %08x\n", PL->reserved);
      printf("        total_length     = %08x\n", PL->total_length);
      printf("    property_table       = %x %p\n",
	      PL->property_table.size, PL->property_table.element);
      printf("        data             = %p\n", PL->data);
      printf("        data_size        = %x\n", PL->data_size);

      print_prop_meta_data2(stdout, PL);
    }

    if (DEH->identifier == X3F_SECi) {
      x3f_image_data_t *ID = &DEH->data_subsection.image_data;
      x3f_huffman_t *HUF = ID->huffman;
      x3f_true_t *TRU = ID->tru;
      x3f_quattro_t *Q = ID->quattro;

      printf("      data_subsection.image_data.\n");
      printf("        type        = %08x\n", ID->type);
      printf("        format      = %08x\n", ID->format);
      printf("        type_format = %08x\n", ID->type_format);
      printf("        columns     = %08x (%d)\n",
             ID->columns, ID->columns);
      printf("        rows        = %08x (%d)\n",
             ID->rows, ID->rows);
      printf("        row_stride  = %08x (%d)\n",
             ID->row_stride, ID->row_stride);

      if (HUF == NULL) {
        printf("        huffman     = %p\n", HUF);
      } else {
        printf("        huffman->\n");
        printf("          mapping     = %x %p\n",
               HUF->mapping.size, HUF->mapping.element);
        printf("          table       = %x %p\n",
               HUF->table.size, HUF->table.element);
        printf("          tree        = %d %p\n",
	       HUF->tree.free_node_index, HUF->tree.nodes);
        printf("          row_offsets = %x %p\n",
               HUF->row_offsets.size, HUF->row_offsets.element);
        printf("          rgb8        = %x %x %p\n",
               HUF->rgb8.columns, HUF->rgb8.rows, HUF->rgb8.data);
        printf("          x3rgb16     = %x %x %p\n",
               HUF->x3rgb16.columns, HUF->x3rgb16.rows, HUF->x3rgb16.data);
      }

      if (TRU == NULL) {
        printf("        tru         = %p\n", TRU);
      } else {
	int i;

        printf("        tru->\n");
        printf("          seed[0]     = %x\n", TRU->seed[0]);
        printf("          seed[1]     = %x\n", TRU->seed[1]);
        printf("          seed[2]     = %x\n", TRU->seed[2]);
        printf("          unknown     = %x\n", TRU->unknown);
        printf("          table       = %x %p\n",
               TRU->table.size, TRU->table.element);
        printf("          plane_size  = %x %p (",
               TRU->plane_size.size, TRU->plane_size.element);
	for (i=0; i<TRU->plane_size.size; i++)
	  printf(" %d", TRU->plane_size.element[i]);
	printf(" )\n");
	printf("          plane_address (");
	for (i=0; i<TRUE_PLANES; i++)
	  printf(" %p", TRU->plane_address[i]);
	printf(" )\n");
	printf("          tree        = %d %p\n",
	       TRU->tree.free_node_index, TRU->tree.nodes);
        printf("          x3rgb16     = %x %x %p\n",
               TRU->x3rgb16.columns, TRU->x3rgb16.rows, TRU->x3rgb16.data);
      }

      if (Q == NULL) {
	printf("        quattro     = %p\n", Q);
      } else {
	int i;

	printf("        quattro->\n");
	printf("          planes (");
	for (i=0; i<TRUE_PLANES; i++) {
	  printf(" %d", Q->plane[i].columns);
	  printf("x%d", Q->plane[i].rows);
	}
	printf(" )\n");
        printf("          unknown     = %x %d\n",
               Q->unknown, Q->unknown);
      }

      printf("        data        = %p\n", ID->data);
    }

    if (DEH->identifier == X3F_SECc) {
      x3f_camf_t *CAMF = &DEH->data_subsection.camf;
      printf("      data_subsection.camf.\n");
      printf("        type             = %x\n", CAMF->type);

      switch (CAMF->type) {
      case 2:
        printf("        type2\n");
        printf("          reserved         = %x\n", CAMF->t2.reserved);
        printf("          infotype         = %08x (%s)\n", CAMF->t2.infotype, x3f_id(CAMF->t2.infotype));
        printf("          infotype_version = %08x\n", CAMF->t2.infotype_version);
        printf("          crypt_key        = %08x\n", CAMF->t2.crypt_key);
        break;
      case 4:
        printf("        type4\n");
        printf("          decoded_data_size= %x\n", CAMF->t4.decoded_data_size);
        printf("          decode_bias      = %x\n", CAMF->t4.decode_bias);
        printf("          block_size       = %x\n", CAMF->t4.block_size);
        printf("          block_count      = %x\n", CAMF->t4.block_count);
        break;
      case 5:
        printf("        type5\n");
        printf("          decoded_data_size= %x\n", CAMF->t5.decoded_data_size);
        printf("          decode_bias      = %x\n", CAMF->t5.decode_bias);
        printf("          unknown2         = %x\n", CAMF->t5.unknown2);
        printf("          unknown3         = %x\n", CAMF->t5.unknown3);
        break;
      default:
        printf("       (Unknown CAMF type)\n");
        printf("          val0             = %x\n", CAMF->tN.val0);
        printf("          val1             = %x\n", CAMF->tN.val1);
        printf("          val2             = %x\n", CAMF->tN.val2);
        printf("          val3             = %x\n", CAMF->tN.val3);
      }

      printf("        data             = %p\n", CAMF->data);
      printf("        data_size        = %x\n", CAMF->data_size);
      printf("        decoding_start   = %p\n", CAMF->decoding_start);
      printf("        decoding_size    = %x\n", CAMF->decoding_size);
      printf("        table            = %x %p\n",
	     CAMF->table.size, CAMF->table.element);
      printf("          tree           = %d %p\n",
	     CAMF->tree.free_node_index, CAMF->tree.nodes);
      printf("        decoded_data     = %p\n", CAMF->decoded_data);
      printf("        decoded_data_size= %x\n", CAMF->decoded_data_size);
      printf("        entry_table      = %x %p\n",
	     CAMF->entry_table.size, CAMF->entry_table.element);

      print_camf_meta_data2(stdout, CAMF);
    }
  }
}

/* extern */ x3f_return_t x3f_dump_meta_data(x3f_t *x3f, char *outfilename)
{
  FILE *f_out = fopen(outfilename, "wb");

  if (f_out == NULL) {
    return X3F_OUTFILE_ERROR;
  }

  print_file_header_meta_data(f_out, x3f);

  print_camf_meta_data(f_out, x3f);

  print_prop_meta_data(f_out, x3f);

  /* We assume that the JPEG meta data is not needed. Therefore JPEG
     is not loaded and no call to any EXIF extraction tool either
     called. */

  fclose(f_out);

  return X3F_OK;
}
