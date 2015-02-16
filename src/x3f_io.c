/* X3F_IO.C - Library for accessing X3F Files.
 *
 * Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_io.h"
#include "x3f_matrix.h"
#include "x3f_dngtags.h"
#include "x3f_denoise.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <tiffio.h>
#include <iconv.h>

/* --------------------------------------------------------------------- */
/* Hacky external flags                                                 */
/* --------------------------------------------------------------------- */

/* extern */ int legacy_offset = 0;
/* extern */ bool_t auto_legacy_offset = 1;

/* extern */ uint32_t max_printed_matrix_elements = 100;

/* --------------------------------------------------------------------- */
/* Huffman Decode Macros                                                 */
/* --------------------------------------------------------------------- */

#define HUF_TREE_MAX_LENGTH 27
#define HUF_TREE_MAX_NODES(_leaves) ((HUF_TREE_MAX_LENGTH+1)*(_leaves))
#define HUF_TREE_GET_LENGTH(_v) (((_v)>>27)&0x1f)
#define HUF_TREE_GET_CODE(_v) ((_v)&0x07ffffff)

/* --------------------------------------------------------------------- */
/* Reading and writing - assuming little endian in the file              */
/* --------------------------------------------------------------------- */

static int x3f_get1(FILE *f)
{
  /* Little endian file */
  return getc(f);
}

static int x3f_get2(FILE *f)
{
  /* Little endian file */
  return (getc(f)<<0) + (getc(f)<<8);
}

static int x3f_get4(FILE *f)
{
  /* Little endian file */
  return (getc(f)<<0) + (getc(f)<<8) + (getc(f)<<16) + (getc(f)<<24);
}

#define FREE(P) do { free(P); (P) = NULL; } while (0)

#define PUT_GET_N(_buffer,_size,_file,_func)			\
  do								\
    {								\
      int _left = _size;					\
      while (_left != 0) {					\
	int _cur = _func(_buffer,1,_left,_file);		\
	if (_cur == 0) {					\
	  fprintf(stderr, "Failure to access file\n");		\
	  exit(1);						\
	}							\
	_left -= _cur;						\
      }								\
    } while(0)

#define GET1(_v) do {(_v) = x3f_get1(I->input.file);} while (0)
#define GET2(_v) do {(_v) = x3f_get2(I->input.file);} while (0)
#define GET4(_v) do {(_v) = x3f_get4(I->input.file);} while (0)
#define GETN(_v,_s) PUT_GET_N(_v,_s,I->input.file,fread)

#define GET_TABLE(_T, _GETX, _NUM)					\
  do {									\
    int _i;								\
    (_T).size = (_NUM);							\
    (_T).element = (void *)realloc((_T).element,			\
				   (_NUM)*sizeof((_T).element[0]));	\
    for (_i = 0; _i < (_T).size; _i++)					\
      _GETX((_T).element[_i]);						\
  } while (0)

#define GET_PROPERTY_TABLE(_T, _NUM)					\
  do {									\
    int _i;								\
    (_T).size = (_NUM);							\
    (_T).element = (void *)realloc((_T).element,			\
				   (_NUM)*sizeof((_T).element[0]));	\
    for (_i = 0; _i < (_T).size; _i++) {				\
      GET4((_T).element[_i].name_offset);				\
      GET4((_T).element[_i].value_offset);				\
    }									\
  } while (0)

#define GET_TRUE_HUFF_TABLE(_T)						\
  do {									\
    int _i;								\
    (_T).element = NULL;						\
    for (_i = 0; ; _i++) {						\
      (_T).size = _i + 1;						\
      (_T).element = (void *)realloc((_T).element,			\
				     (_i + 1)*sizeof((_T).element[0]));	\
      GET1((_T).element[_i].code_size);					\
      GET1((_T).element[_i].code);					\
      if ((_T).element[_i].code_size == 0) break;			\
    }									\
  } while (0)

#define PUT1(_v) x3f_put1(I->output.file, _v)
#define PUT2(_v) x3f_put2(I->output.file, _v)
#define PUT4(_v) x3f_put4(I->output.file, _v)
#define PUTN(_v,_s) PUT_GET_N(_v,_s,I->output.file,fwrite)

#define PUT_TABLE(_T, _PUTX)			\
  do {						\
    int _i;					\
    if ((_T).size == 0) break;			\
    for (_i = 0; _i < (_T).size; _i++)		\
      _PUTX((_T).element[_i]);			\
  } while (0)

#define PUT_PROPERTY_TABLE(_T)					\
  do {								\
    int _i;							\
    if ((_T).size == 0) break;					\
    for (_i = 0; _i < (_T).size; _i++) {			\
      PUT4((_T).element[_i].name_offset);			\
      PUT4((_T).element[_i].value_offset);			\
    }								\
  } while (0)

#define PUT_TRUE_HUFF_TABLE(_T)				\
  do {							\
    int _i;						\
    if ((_T).size == 0) break;				\
    for (_i = 0; _i < (_T).size; _i++) {		\
      PUT1((_T).element[_i].code_size);			\
      PUT1((_T).element[_i].code);			\
    }							\
  } while (0)


#if 0
/* --------------------------------------------------------------------- */
/* Converting - ingeger vs memory - assuming little endian in the memory */
/* --------------------------------------------------------------------- */

static void x3f_convert2(void *to, void *from)
{
  uint8_t *f = (uint8_t *)from;
  uint16_t *t = (uint16_t *)to;

  /* Little endian memory */
  *t = (uint16_t)((*(f+0)<<0) + (*(f+1)<<8));
}

static void x3f_convert4(void *to, void *from)
{
  uint8_t *f = (uint8_t *)from;
  uint16_t *t = (uint16_t *)to;

  /* Little endian memory */
  *t = (uint32_t)((*(f+0)<<0) + (*(f+1)<<8) + (*(f+2)<<16) + (*(f+3)<<24));
}

#define CONV2(_v, _p) do {x3f_conv2(_p, _v);} while (0)
#define CONV4(_v, _p) do {x3f_conv4(_p, _v);} while (0)
#endif


/* --------------------------------------------------------------------- */
/* Allocating Huffman tree help data                                   */
/* --------------------------------------------------------------------- */

static void cleanup_huffman_tree(x3f_hufftree_t *HTP)
{
  free(HTP->nodes);
}

static void new_huffman_tree(x3f_hufftree_t *HTP, int bits)
{
  int leaves = 1<<bits;

  HTP->free_node_index = 0;
  HTP->nodes = (x3f_huffnode_t *)
    calloc(1, HUF_TREE_MAX_NODES(leaves)*sizeof(x3f_huffnode_t));
}

/* --------------------------------------------------------------------- */
/* Allocating TRUE engine RAW help data                                  */
/* --------------------------------------------------------------------- */

static void cleanup_true(x3f_true_t **TRUP)
{
  x3f_true_t *TRU = *TRUP;

  if (TRU == NULL) return;

  printf("Cleanup TRUE data\n");

  FREE(TRU->table.element);
  FREE(TRU->plane_size.element);
  cleanup_huffman_tree(&TRU->tree);
  FREE(TRU->x3rgb16.element);

  FREE(TRU);

  *TRUP = NULL;
}

static x3f_true_t *new_true(x3f_true_t **TRUP)
{
  x3f_true_t *TRU = (x3f_true_t *)calloc(1, sizeof(x3f_true_t));

  cleanup_true(TRUP);

  TRU->table.size = 0;
  TRU->table.element = NULL;
  TRU->plane_size.size = 0;
  TRU->plane_size.element = NULL;
  TRU->tree.nodes = NULL;
  TRU->x3rgb16.size = 0;
  TRU->x3rgb16.element = NULL;

  *TRUP = TRU;

  return TRU;
}

static void cleanup_quattro(x3f_quattro_t **QP)
{
  x3f_quattro_t *Q = *QP;

  if (Q == NULL) return;

  printf("Cleanup/Free Quattro\n");

  /* Here is a place where you can clean up stuff */

  FREE(Q);

  *QP = NULL;
}

static x3f_quattro_t *new_quattro(x3f_quattro_t **QP)
{
  x3f_quattro_t *Q = (x3f_quattro_t *)calloc(1, sizeof(x3f_quattro_t));
  int i;

  cleanup_quattro(QP);

  for (i=0; i<TRUE_PLANES; i++) {
    Q->plane[i].columns = 0;
    Q->plane[i].rows = 0;
  }

  Q->unknown = 0;

  *QP = Q;

  return Q;
}

/* --------------------------------------------------------------------- */
/* Allocating Huffman engine help data                                   */
/* --------------------------------------------------------------------- */

static void cleanup_huffman(x3f_huffman_t **HUFP)
{
  x3f_huffman_t *HUF = *HUFP;

  if (HUF == NULL) return;

  printf("Cleanup Huffman\n");

  FREE(HUF->mapping.element);
  FREE(HUF->table.element);
  cleanup_huffman_tree(&HUF->tree);
  FREE(HUF->row_offsets.element);
  FREE(HUF->rgb8.element);
  FREE(HUF->x3rgb16.element);
  FREE(HUF);

  *HUFP = NULL;
}

static x3f_huffman_t *new_huffman(x3f_huffman_t **HUFP)
{
  x3f_huffman_t *HUF = (x3f_huffman_t *)calloc(1, sizeof(x3f_huffman_t));

  cleanup_huffman(HUFP);

  /* Set all not read data block pointers to NULL */
  HUF->mapping.size = 0;
  HUF->mapping.element = NULL;
  HUF->table.size = 0;
  HUF->table.element = NULL;
  HUF->tree.nodes = NULL;
  HUF->row_offsets.size = 0;
  HUF->row_offsets.element = NULL;
  HUF->rgb8.size = 0;
  HUF->rgb8.element = NULL;
  HUF->x3rgb16.size = 0;
  HUF->x3rgb16.element = NULL;

  *HUFP = HUF;

  return HUF;
}


/* --------------------------------------------------------------------- */
/* Creating a new x3f structure from file                                */
/* --------------------------------------------------------------------- */

/* extern */ x3f_t *x3f_new_from_file(FILE *infile)
{
  x3f_t *x3f = (x3f_t *)calloc(1, sizeof(x3f_t));
  x3f_info_t *I = NULL;
  x3f_header_t *H = NULL;
  x3f_directory_section_t *DS = NULL;
  int i, d;

  I = &x3f->info;
  I->error = NULL;
  I->input.file = infile;
  I->output.file = NULL;

  if (infile == NULL) {
    I->error = "No infile";
    return x3f;
  }

  /* Read file header */
  H = &x3f->header;
  fseek(infile, 0, SEEK_SET);
  GET4(H->identifier);

  if (H->identifier != X3F_FOVb) {
    fprintf(stderr, "Faulty file type\n");
    x3f_delete(x3f);
    return NULL;
  }

  GET4(H->version);
  GETN(H->unique_identifier, SIZE_UNIQUE_IDENTIFIER);
  GET4(H->mark_bits);
  GET4(H->columns);
  GET4(H->rows);
  GET4(H->rotation);
  if (H->version > X3F_VERSION_2_0) {
    GETN(H->white_balance, SIZE_WHITE_BALANCE);
    GETN(H->extended_types, NUM_EXT_DATA);
    for (i=0; i<NUM_EXT_DATA; i++)
      GET4(H->extended_data[i]);
  }

  /* Go to the beginning of the directory */
  fseek(infile, -4, SEEK_END);
  fseek(infile, x3f_get4(infile), SEEK_SET);

  /* Read the directory header */
  DS = &x3f->directory_section;
  GET4(DS->identifier);
  GET4(DS->version);
  GET4(DS->num_directory_entries);

  if (DS->num_directory_entries > 0) {
    size_t size = DS->num_directory_entries * sizeof(x3f_directory_entry_t);
    DS->directory_entry = (x3f_directory_entry_t *)calloc(1, size);
  }

  /* Traverse the directory */
  for (d=0; d<DS->num_directory_entries; d++) {
    x3f_directory_entry_t *DE = &DS->directory_entry[d];
    x3f_directory_entry_header_t *DEH = &DE->header;
    uint32_t save_dir_pos;

    /* Read the directory entry info */
    GET4(DE->input.offset);
    GET4(DE->input.size);

    DE->output.offset = 0;
    DE->output.size = 0;

    GET4(DE->type);

    /* Save current pos and go to the entry */
    save_dir_pos = ftell(infile);
    fseek(infile, DE->input.offset, SEEK_SET);

    /* Read the type independent part of the entry header */
    DEH = &DE->header;
    GET4(DEH->identifier);
    GET4(DEH->version);

    /* NOTE - the tests below could be made on DE->type instead */

    if (DEH->identifier == X3F_SECp) {
      x3f_property_list_t *PL = &DEH->data_subsection.property_list;

      /* Read the property part of the header */
      GET4(PL->num_properties);
      GET4(PL->character_format);
      GET4(PL->reserved);
      GET4(PL->total_length);

      /* Set all not read data block pointers to NULL */
      PL->data = NULL;
      PL->data_size = 0;
    }

    if (DEH->identifier == X3F_SECi) {
      x3f_image_data_t *ID = &DEH->data_subsection.image_data;

      /* Read the image part of the header */
      GET4(ID->type);
      GET4(ID->format);
      ID->type_format = (ID->type << 16) + (ID->format);
      GET4(ID->columns);
      GET4(ID->rows);
      GET4(ID->row_stride);

      /* Set all not read data block pointers to NULL */
      ID->huffman = NULL;

      ID->data = NULL;
      ID->data_size = 0;
    }

    if (DEH->identifier == X3F_SECc) {
      x3f_camf_t *CAMF = &DEH->data_subsection.camf;

      /* Read the CAMF part of the header */
      GET4(CAMF->type);
      GET4(CAMF->tN.val0);
      GET4(CAMF->tN.val1);
      GET4(CAMF->tN.val2);
      GET4(CAMF->tN.val3);

      /* Set all not read data block pointers to NULL */
      CAMF->data = NULL;
      CAMF->data_size = 0;

      /* Set all not allocated help pointers to NULL */
      CAMF->table.element = NULL;
      CAMF->table.size = 0;
      CAMF->tree.nodes = NULL;
      CAMF->decoded_data = NULL;
      CAMF->decoded_data_size = 0;
      CAMF->entry_table.element = NULL;
      CAMF->entry_table.size = 0;
    }

    /* Reset the file pointer back to the directory */
    fseek(infile, save_dir_pos, SEEK_SET);
  }

  return x3f;
}


#if 0 /* Not used display functions */

/* --------------------------------------------------------------------- */
/* Pretty print UTF 16                                        */
/* --------------------------------------------------------------------- */

static char display_char(char c)
{
  if (c == 0)
    return ',';
  if (c >= '0' && c <= '9')
    return c;
  if (c >= 'a' && c <= 'z')
    return c;
  if (c >= 'A' && c <= 'Z')
    return c;
  if (c == '_')
    return c;
  if (c == '.')
    return c;
  if (c == '-')
    return c;
  if (c == '/')
    return c;
  if (c == ' ')
    return c;

  return '?';
}

static char *display_chars(char *str, char *buffer, int size)
{
  int i;
  char *b = buffer;

  for (i=0; i<size; i++) {
    *b++ = display_char(*str++);
  }

  *b = 0;

  return buffer;
}

static char *display_char_string(char *str, char *buffer)
{
  char *b = buffer;

  while (*str != 0x0) {
    *b++ = display_char(*str++);
  }

  *b = 0;

  return buffer;
}

static char *display_utf16s(utf16_t *str, char *buffer, int size)
{
  int i;
  char *b = buffer;

  for (i=0; i<size; i += 2) { 

    char *chr = (char *)str;

    char chr1 = *chr;
    char chr2 = *(chr+1);

    if (chr1)
      *b++ = display_char(chr1);
    if (chr2)
      *b++ = display_char(chr2);

    str++;
  }

  *b = 0;

  return buffer;
}

static char *display_utf16_string(utf16_t *str, char *buffer)
{
  char *b = buffer;

  while (*str != 0x00) {

    char *chr = (char *)str;

    char chr1 = *chr;
    char chr2 = *(chr+1);

    if (chr1)
      *b++ = display_char(chr1);
    if (chr2)
      *b++ = display_char(chr2);

    str++;
  }

  *b = 0;

  return buffer;
}

#endif /* Not used display functions */

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
        printf("          rgb8        = %x %p\n",
               HUF->rgb8.size, HUF->rgb8.element);
        printf("          x3rgb16     = %x %p\n",
               HUF->x3rgb16.size, HUF->x3rgb16.element);
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
        printf("          x3rgb16     = %x %p\n",
               TRU->x3rgb16.size, TRU->x3rgb16.element);
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


/* --------------------------------------------------------------------- */
/* Clean up an x3f structure                                             */
/* --------------------------------------------------------------------- */

static void free_camf_entry(camf_entry_t *entry)
{
  FREE(entry->property_name);
  FREE(entry->property_value);
  FREE(entry->matrix_decoded);
  FREE(entry->matrix_dim_entry);
}

/* extern */ x3f_return_t x3f_delete(x3f_t *x3f)
{
  x3f_directory_section_t *DS;
  int d;

  if (x3f == NULL)
    return X3F_ARGUMENT_ERROR;

  printf("X3F Delete\n");

  DS = &x3f->directory_section;

  for (d=0; d<DS->num_directory_entries; d++) {
    x3f_directory_entry_t *DE = &DS->directory_entry[d];
    x3f_directory_entry_header_t *DEH = &DE->header;

    if (DEH->identifier == X3F_SECp) {
      x3f_property_list_t *PL = &DEH->data_subsection.property_list;
      int i;

      for (i=0; i<PL->property_table.size; i++) {
	FREE(PL->property_table.element[i].name_utf8);
	FREE(PL->property_table.element[i].value_utf8);
      }

      FREE(PL->property_table.element);
      FREE(PL->data);
    }

    if (DEH->identifier == X3F_SECi) {
      x3f_image_data_t *ID = &DEH->data_subsection.image_data;

      cleanup_huffman(&ID->huffman);

      cleanup_true(&ID->tru);

      cleanup_quattro(&ID->quattro);

      FREE(ID->data);
    }

    if (DEH->identifier == X3F_SECc) {
      x3f_camf_t *CAMF = &DEH->data_subsection.camf;
      int i;

      FREE(CAMF->data);
      FREE(CAMF->table.element);
      cleanup_huffman_tree(&CAMF->tree);
      FREE(CAMF->decoded_data);
      for (i=0; i < CAMF->entry_table.size; i++) {
	free_camf_entry(&CAMF->entry_table.element[i]);
      }
      FREE(CAMF->entry_table.element);
    }
  }

  FREE(DS->directory_entry);
  FREE(x3f);

  return X3F_OK;
}


/* --------------------------------------------------------------------- */
/* Getting a reference to a directory entry                              */
/* --------------------------------------------------------------------- */

/* TODO: all those only get the first instance */

static x3f_directory_entry_t *x3f_get(x3f_t *x3f,
                                      uint32_t type,
                                      uint32_t image_type)
{
  x3f_directory_section_t *DS;
  int d;

  if (x3f == NULL) return NULL;

  DS = &x3f->directory_section;

  for (d=0; d<DS->num_directory_entries; d++) {
    x3f_directory_entry_t *DE = &DS->directory_entry[d];
    x3f_directory_entry_header_t *DEH = &DE->header;

    if (DEH->identifier == type) {
      switch (DEH->identifier) {
      case X3F_SECi:
        {
          x3f_image_data_t *ID = &DEH->data_subsection.image_data;

          if (ID->type_format == image_type)
            return DE;
        }
        break;
      default:
        return DE;
      }
    }
  }

  return NULL;
}

/* extern */ x3f_directory_entry_t *x3f_get_raw(x3f_t *x3f)
{
  x3f_directory_entry_t *DE;

  if ((DE = x3f_get(x3f, X3F_SECi, X3F_IMAGE_RAW_HUFFMAN_X530)) != NULL)
    return DE;

  if ((DE = x3f_get(x3f, X3F_SECi, X3F_IMAGE_RAW_HUFFMAN_10BIT)) != NULL)
    return DE;

  if ((DE = x3f_get(x3f, X3F_SECi, X3F_IMAGE_RAW_TRUE)) != NULL)
    return DE;

  if ((DE = x3f_get(x3f, X3F_SECi, X3F_IMAGE_RAW_MERRILL)) != NULL)
    return DE;

  if ((DE = x3f_get(x3f, X3F_SECi, X3F_IMAGE_RAW_QUATTRO)) != NULL)
    return DE;

  return NULL;
}

/* extern */ x3f_directory_entry_t *x3f_get_thumb_plain(x3f_t *x3f)
{
  return x3f_get(x3f, X3F_SECi, X3F_IMAGE_THUMB_PLAIN);
}

/* extern */ x3f_directory_entry_t *x3f_get_thumb_huffman(x3f_t *x3f)
{
  return x3f_get(x3f, X3F_SECi, X3F_IMAGE_THUMB_HUFFMAN);
}

/* extern */ x3f_directory_entry_t *x3f_get_thumb_jpeg(x3f_t *x3f)
{
  return x3f_get(x3f, X3F_SECi, X3F_IMAGE_THUMB_JPEG);
}

/* extern */ x3f_directory_entry_t *x3f_get_camf(x3f_t *x3f)
{
  return x3f_get(x3f, X3F_SECc, 0);
}

/* extern */ x3f_directory_entry_t *x3f_get_prop(x3f_t *x3f)
{
  return x3f_get(x3f, X3F_SECp, 0);
}

/* For some obscure reason, the bit numbering is weird. It is
   generally some kind of "big endian" style - e.g. the bit 7 is the
   first in a byte and bit 31 first in a 4 byte int. For patterns in
   the huffman pattern table, bit 27 is the first bit and bit 26 the
   next one. */

#define PATTERN_BIT_POS(_len, _bit) ((_len) - (_bit) - 1)
#define MEMORY_BIT_POS(_bit) PATTERN_BIT_POS(8, _bit)


/* --------------------------------------------------------------------- */
/* Huffman Decode                                                        */
/* --------------------------------------------------------------------- */

/* Make the huffman tree */

#ifdef DBG_PRNT
static char *display_code(int length, uint32_t code, char *buffer)
{
  int i;

  for (i=0; i<length; i++) {
    int pos = PATTERN_BIT_POS(length, i);
    buffer[i] = ((code>>pos)&1) == 0 ? '0' : '1';
  }

  buffer[i] = 0;

  return buffer;
}
#endif

static x3f_huffnode_t *new_node(x3f_hufftree_t *tree)
{
  x3f_huffnode_t *t = &tree->nodes[tree->free_node_index];

  t->branch[0] = NULL;
  t->branch[1] = NULL;
  t->leaf = UNDEFINED_LEAF;

  tree->free_node_index++;

  return t;
}

static void add_code_to_tree(x3f_hufftree_t *tree,
                             int length, uint32_t code, uint32_t value)
{
  int i;

  x3f_huffnode_t *t = tree->nodes;

  for (i=0; i<length; i++) {
    int pos = PATTERN_BIT_POS(length, i);
    int bit = (code>>pos)&1;
    x3f_huffnode_t *t_next = t->branch[bit];

    if (t_next == NULL)
      t_next = t->branch[bit] = new_node(tree);

    t = t_next;
  }

  t->leaf = value;
}

static void populate_true_huffman_tree(x3f_hufftree_t *tree,
				       x3f_true_huffman_t *table)
{
  int i;

  new_node(tree);

  for (i=0; i<table->size; i++) {
    x3f_true_huffman_element_t *element = &table->element[i];
    uint32_t length = element->code_size;

    if (length != 0) {
      /* add_code_to_tree wants the code right adjusted */
      uint32_t code = ((element->code) >> (8 - length)) & 0xff;
      uint32_t value = i;

      add_code_to_tree(tree, length, code, value);

#ifdef DBG_PRNT
      {
        char buffer[100];

        printf("H %5d : %5x : %5d : %02x %08x (%08x) (%s)\n",
               i, i, value, length, code, value,
               display_code(length, code, buffer));
      }
#endif
    }
  }
}

static void populate_huffman_tree(x3f_hufftree_t *tree,
				  x3f_table32_t *table,
				  x3f_table16_t *mapping)
{
  int i;

  new_node(tree);

  for (i=0; i<table->size; i++) {
    uint32_t element = table->element[i];

    if (element != 0) {
      uint32_t length = HUF_TREE_GET_LENGTH(element);
      uint32_t code = HUF_TREE_GET_CODE(element);
      uint32_t value;

      /* If we have a valid mapping table - then the value from the
         mapping table shall be used. Otherwise we use the current
         index in the table as value. */
      if (table->size == mapping->size)
        value = mapping->element[i];
      else
        value = i;

      add_code_to_tree(tree, length, code, value);

#ifdef DBG_PRNT
      {
        char buffer[100];

        printf("H %5d : %5x : %5d : %02x %08x (%08x) (%s)\n",
               i, i, value, length, code, element,
               display_code(length, code, buffer));
      }
#endif
    }
  }
}

#ifdef DBG_PRNT
static void print_huffman_tree(x3f_huffnode_t *t, int length, uint32_t code)
{
  char buf1[100];
  char buf2[100];

  printf("%*s (%s,%s) %s (%s)\n",
         length, length < 1 ? "-" : (code&1) ? "1" : "0",
         t->branch[0]==NULL ? "-" : "0",
         t->branch[1]==NULL ? "-" : "1",
         t->leaf==UNDEFINED_LEAF ? "-" : (sprintf(buf1, "%x", t->leaf),buf1),
         display_code(length, code, buf2));

  code = code << 1;
  if (t->branch[0]) print_huffman_tree(t->branch[0], length+1, code+0);
  if (t->branch[1]) print_huffman_tree(t->branch[1], length+1, code+1);
}
#endif

/* Help machinery for reading bits in a memory */

typedef struct bit_state_s {
  uint8_t *next_address;
  uint8_t bit_offset;
  uint8_t bits[8];
} bit_state_t;

static void set_bit_state(bit_state_t *BS, uint8_t *address)
{
  BS->next_address = address;
  BS->bit_offset = 8;
}

static uint8_t get_bit(bit_state_t *BS)
{
  if (BS->bit_offset == 8) {
    uint8_t byte = *BS->next_address;
    int i;

    for (i=7; i>= 0; i--) {
      BS->bits[i] = byte&1;
      byte = byte >> 1;
    }
    BS->next_address++;
    BS->bit_offset = 0;
  }

  return BS->bits[BS->bit_offset++];
}

/* Decode use the TRUE algorithm */

static int32_t get_true_diff(bit_state_t *BS, x3f_hufftree_t *HTP)
{
  int32_t diff;
  x3f_huffnode_t *node = &HTP->nodes[0];
  uint8_t bits;

  while (node->branch[0] != NULL || node->branch[1] != NULL) {
    uint8_t bit = get_bit(BS);
    x3f_huffnode_t *new_node = node->branch[bit];

    node = new_node;
    if (node == NULL) {
      fprintf(stderr, "Huffman coding got unexpected bit\n");
      return 0;
    }
  }

  bits = node->leaf;

  if (bits == 0)
    diff = 0;
  else {
    uint8_t first_bit = get_bit(BS);
    int i;

    diff = first_bit;

    for (i=1; i<bits; i++)
      diff = (diff << 1) + get_bit(BS);

    if (first_bit == 0)
      diff -= (1<<bits) - 1;
  }

  return diff;
}

/* This code (that decodes one of the X3F color planes, really is a
   decoding of a compression algorithm suited for Bayer CFA data. In
   Bayer CFA the data is divided into 2x2 squares that represents
   (R,G1,G2,B) data. Those four positions are (in this compression)
   treated as one data stream each, where you store the differences to
   previous data in the stream. The reason for this is, of course,
   that the date is more often than not near to the next data in a
   stream that represents the same color. */

/* TODO: write more about the compression */

static void true_decode_one_color(x3f_image_data_t *ID, int color)
{
  x3f_true_t *TRU = ID->tru;
  uint32_t seed = TRU->seed[color]; /* TODO : Is this correct ? */
  int row;

  x3f_hufftree_t *tree = &TRU->tree;
  bit_state_t BS;

  int32_t row_start_acc[2][2];
  uint32_t rows = ID->rows;
  uint32_t cols = ID->columns;
  uint32_t out_cols = ID->columns;
  bool_t quattro_modified = 0;

  uint16_t *dst = TRU->x3rgb16.element + color;

  printf("TRUE decode one color (%d) rows=%d cols=%d\n",
	 color, rows, cols);

  set_bit_state(&BS, TRU->plane_address[color]);

  row_start_acc[0][0] = seed;
  row_start_acc[0][1] = seed;
  row_start_acc[1][0] = seed;
  row_start_acc[1][1] = seed;

  if (ID->type_format == X3F_IMAGE_RAW_QUATTRO) {
    uint32_t qrows = ID->quattro->plane[color].rows;
    uint32_t qcols = ID->quattro->plane[color].columns;

    if (qcols != cols) { /* Strange test, due to X3F file format uses
			    the same type_format for binned data. */
      rows = qrows;
      cols = qcols;
      quattro_modified = 1;
      printf("Quattro modified decode one color (%d) rows=%d cols=%d\n",
	     color, rows, cols);
    }
  }

  for (row = 0; row < rows; row++) {
    int col;
    bool_t odd_row = row&1;
    int32_t acc[2];

    for (col = 0; col < cols; col++) {
      bool_t odd_col = col&1;
      int32_t diff = get_true_diff(&BS, tree);
      int32_t prev = col < 2 ?
	row_start_acc[odd_row][odd_col] :
	acc[odd_col];
      int32_t value = prev + diff;

      acc[odd_col] = value;
      if (col < 2)
	row_start_acc[odd_row][odd_col] = value;

      if (col < out_cols) { /* For quattro the input may be larger
			       than the output */
	*dst = value;
	dst += 3;
      }
    }
  }

  if (quattro_modified && (color < 2)) {
    /* The pixels in the layers with lower resolution are duplicated
       to four values. This is done in place, therefore done backwards */

    uint16_t *base = TRU->x3rgb16.element + color;

    printf("Expand Quattro layer %d\n", color);

    for (row = rows-1; row >= 0; row--) {
      int col;

      for (col = cols-1; col >= 0; col--) {
	uint16_t val = base[3*(cols*row + col)];

	base[3*(2*cols*(2*row+1) + 2*col+1)] = val;
	base[3*(2*cols*(2*row+1) + 2*col+0)] = val;
	base[3*(2*cols*(2*row+0) + 2*col+1)] = val;
	base[3*(2*cols*(2*row+0) + 2*col+0)] = val;
      }
    }

    printf("Expanded\n");
  }

  /* TODO - here should really non square binning be expanded */
}

static void true_decode(x3f_info_t *I,
			x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  int color;

  for (color = 0; color < 3; color++) {
    true_decode_one_color(ID, color);
  }
}

/* Decode use the huffman tree */

static int32_t get_huffman_diff(bit_state_t *BS, x3f_hufftree_t *HTP)
{
  int32_t diff;
  x3f_huffnode_t *node = &HTP->nodes[0];

  while (node->branch[0] != NULL || node->branch[1] != NULL) {
    uint8_t bit = get_bit(BS);
    x3f_huffnode_t *new_node = node->branch[bit];

    node = new_node;
    if (node == NULL) {
      fprintf(stderr, "Huffman coding got unexpected bit\n");
      return 0;
    }
  }

  diff = node->leaf;

  return diff;
}

static void huffman_decode_row(x3f_info_t *I,
                               x3f_directory_entry_t *DE,
                               int bits,
                               int row,
                               int offset,
                               int *minimum)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_huffman_t *HUF = ID->huffman;

  int16_t c[3] = {offset,offset,offset};
  int col;
  bit_state_t BS;

  set_bit_state(&BS, ID->data + HUF->row_offsets.element[row]);

  for (col = 0; col < ID->columns; col++) {
    int color;

    for (color = 0; color < 3; color++) {
      uint16_t c_fix;

      c[color] += get_huffman_diff(&BS, &HUF->tree);
      if (c[color] < 0) {
        c_fix = 0;
        if (c[color] < *minimum)
          *minimum = c[color];
      } else {
        c_fix = c[color];
      }

      switch (ID->type_format) {
      case X3F_IMAGE_RAW_HUFFMAN_X530:
      case X3F_IMAGE_RAW_HUFFMAN_10BIT:
        HUF->x3rgb16.element[3*(row*ID->columns + col) + color] = (uint16_t)c_fix;
        break;
      case X3F_IMAGE_THUMB_HUFFMAN:
        HUF->rgb8.element[3*(row*ID->columns + col) + color] = (uint8_t)c_fix;
        break;
      default:
        fprintf(stderr, "Unknown huffman image type\n");
      }
    }
  }
}

static void huffman_decode(x3f_info_t *I,
                           x3f_directory_entry_t *DE,
                           int bits)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;

  int row;
  int minimum = 0;
  int offset = legacy_offset;

  printf("Huffman decode with offset: %d\n", offset);
  for (row = 0; row < ID->rows; row++)
    huffman_decode_row(I, DE, bits, row, offset, &minimum);

  if (auto_legacy_offset && minimum < 0) {
    offset = -minimum;
    printf("Redo with offset: %d\n", offset);
    for (row = 0; row < ID->rows; row++)
      huffman_decode_row(I, DE, bits, row, offset, &minimum);
  }
}

static int32_t get_simple_diff(x3f_huffman_t *HUF, uint16_t index)
{
  if (HUF->mapping.size == 0)
    return index;
  else
    return HUF->mapping.element[index];
}

static void simple_decode_row(x3f_info_t *I,
                              x3f_directory_entry_t *DE,
                              int bits,
                              int row,
                              int row_stride)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_huffman_t *HUF = ID->huffman;

  uint32_t *data = (uint32_t *)(ID->data + row*row_stride);

  uint16_t c[3] = {0,0,0};
  int col;

  uint32_t mask = 0;

  switch (bits) {
  case 8:
    mask = 0x0ff;
    break;
  case 9:
    mask = 0x1ff;
    break;
  case 10:
    mask = 0x3ff;
    break;
  case 11:
    mask = 0x7ff;
    break;
  case 12:
    mask = 0xfff;
    break;
  default:
    fprintf(stderr, "Unknown number of bits: %d\n", bits);
    mask = 0;
    break;
  }

  for (col = 0; col < ID->columns; col++) {
    int color;
    uint32_t val = data[col];

    for (color = 0; color < 3; color++) {
      uint16_t c_fix;
      c[color] += get_simple_diff(HUF, (val>>(color*bits))&mask);

      switch (ID->type_format) {
      case X3F_IMAGE_RAW_HUFFMAN_X530:
      case X3F_IMAGE_RAW_HUFFMAN_10BIT:
        c_fix = (int16_t)c[color] > 0 ? c[color] : 0;

        HUF->x3rgb16.element[3*(row*ID->columns + col) + color] = c_fix;
        break;
      case X3F_IMAGE_THUMB_HUFFMAN:
        c_fix = (int8_t)c[color] > 0 ? c[color] : 0;

        HUF->rgb8.element[3*(row*ID->columns + col) + color] = c_fix;
        break;
      default:
        fprintf(stderr, "Unknown huffman image type\n");
      }
    }
  }
}

static void simple_decode(x3f_info_t *I,
                          x3f_directory_entry_t *DE,
                          int bits,
                          int row_stride)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;

  int row;

  for (row = 0; row < ID->rows; row++)
    simple_decode_row(I, DE, bits, row, row_stride);
}

/* --------------------------------------------------------------------- */
/* Loading the data in a directory entry                                 */
/* --------------------------------------------------------------------- */

/* First you set the offset to where to start reading the data ... */

static void read_data_set_offset(x3f_info_t *I,
                                 x3f_directory_entry_t *DE,
                                 uint32_t header_size)
{
  uint32_t i_off = DE->input.offset + header_size;

  fseek(I->input.file, i_off, SEEK_SET);
}

/* ... then you read the data, block for block */

static uint32_t read_data_block(void **data,
                                x3f_info_t *I,
                                x3f_directory_entry_t *DE,
                                uint32_t footer)
{
  uint32_t size =
    DE->input.size + DE->input.offset - ftell(I->input.file) - footer;

  *data = (void *)malloc(size);

  GETN(*data, size);

  return size;
}

static void x3f_load_image_verbatim(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;

  printf("Load image verbatim\n");

  ID->data_size = read_data_block(&ID->data, I, DE, 0);
}

static char *utf16le_to_utf8(utf16_t *str)
{
  iconv_t ic = iconv_open("UTF-8", "UTF-16LE");
  size_t isize, osize;
  char *buf, *ibuf, *obuf;

  assert(ic != (iconv_t)-1);
  
  for (isize=0; str[isize]; isize++);
  isize *= 2;			/* Size in bytes */
  osize = 2*isize;		/* Worst case scenario */

  buf = malloc(osize+1);
  ibuf = (char *)str;
  obuf = buf;

  assert(iconv(ic, &ibuf, &isize, &obuf, &osize) != -1);
  *obuf = 0;

  iconv_close(ic);

  return realloc(buf, obuf-buf+1);
}

static void x3f_load_property_list(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_property_list_t *PL = &DEH->data_subsection.property_list;
  int i;

  read_data_set_offset(I, DE, X3F_PROPERTY_LIST_HEADER_SIZE);

  GET_PROPERTY_TABLE(PL->property_table, PL->num_properties);

  PL->data_size = read_data_block(&PL->data, I, DE, 0);

  for (i=0; i<PL->num_properties; i++) {
    x3f_property_t *P = &PL->property_table.element[i];

    P->name = ((utf16_t *)PL->data + P->name_offset);
    P->value = ((utf16_t *)PL->data + P->value_offset);
    P->name_utf8 = utf16le_to_utf8(P->name);
    P->value_utf8 = utf16le_to_utf8(P->value);
  }
}

static void x3f_load_true(x3f_info_t *I,
			  x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_true_t *TRU = new_true(&ID->tru);
  x3f_quattro_t *Q = NULL;
  int i;

  if (ID->type_format == X3F_IMAGE_RAW_QUATTRO) {
    printf("Load Quattro extra info\n");

    Q = new_quattro(&ID->quattro);

    for (i=0; i<TRUE_PLANES; i++) {
      GET2(Q->plane[i].columns);
      GET2(Q->plane[i].rows);
    }
  }

  printf("Load TRUE\n");

  /* Read TRUE header data */
  GET2(TRU->seed[0]);
  GET2(TRU->seed[1]);
  GET2(TRU->seed[2]);
  GET2(TRU->unknown);
  GET_TRUE_HUFF_TABLE(TRU->table);

  if (ID->type_format == X3F_IMAGE_RAW_QUATTRO) {

    printf("Load Quattro extra info 2\n");

    GET4(Q->unknown);
  }

  GET_TABLE(TRU->plane_size, GET4, TRUE_PLANES);

  /* Read image data */
  ID->data_size = read_data_block(&ID->data, I, DE, 0);

  /* TODO: can it be fewer than 8 bits? Maybe taken from TRU->table? */
  new_huffman_tree(&TRU->tree, 8);

  populate_true_huffman_tree(&TRU->tree, &TRU->table);

#ifdef DBG_PRNT
  print_huffman_tree(TRU->tree.nodes, 0, 0);
#endif

  TRU->plane_address[0] = ID->data;
  for (i=1; i<TRUE_PLANES; i++)
    TRU->plane_address[i] =
      TRU->plane_address[i-1] +
      (((TRU->plane_size.element[i-1] + 15) / 16) * 16);

  TRU->x3rgb16.size = ID->columns * ID->rows * 3;
  TRU->x3rgb16.element =
    (uint16_t *)malloc(sizeof(uint16_t)*TRU->x3rgb16.size);

  true_decode(I, DE);
}

static void x3f_load_huffman_compressed(x3f_info_t *I,
                                        x3f_directory_entry_t *DE,
                                        int bits,
                                        int use_map_table)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_huffman_t *HUF = ID->huffman;
  int table_size = 1<<bits;
  int row_offsets_size = ID->rows * sizeof(HUF->row_offsets.element[0]);

  printf("Load huffman compressed\n");

  GET_TABLE(HUF->table, GET4, table_size);

  ID->data_size = read_data_block(&ID->data, I, DE, row_offsets_size);

  GET_TABLE(HUF->row_offsets, GET4, ID->rows);

  printf("Make huffman tree ...\n");
  new_huffman_tree(&HUF->tree, bits);
  populate_huffman_tree(&HUF->tree, &HUF->table, &HUF->mapping);
  printf("... DONE\n");

#ifdef DBG_PRNT
  print_huffman_tree(HUF->tree.nodes, 0, 0);
#endif

  huffman_decode(I, DE, bits);
}

static void x3f_load_huffman_not_compressed(x3f_info_t *I,
                                            x3f_directory_entry_t *DE,
                                            int bits,
                                            int use_map_table,
                                            int row_stride)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;

  printf("Load huffman not compressed\n");

  ID->data_size = read_data_block(&ID->data, I, DE, 0);

  simple_decode(I, DE, bits, row_stride);
}

static void x3f_load_huffman(x3f_info_t *I,
                             x3f_directory_entry_t *DE,
                             int bits,
                             int use_map_table,
                             int row_stride)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  x3f_huffman_t *HUF = new_huffman(&ID->huffman);

  if (use_map_table) {
    int table_size = 1<<bits;

    GET_TABLE(HUF->mapping, GET2, table_size);
  }

  switch (ID->type_format) {
  case X3F_IMAGE_RAW_HUFFMAN_X530:
  case X3F_IMAGE_RAW_HUFFMAN_10BIT:
    HUF->x3rgb16.size = ID->columns * ID->rows * 3;
    HUF->x3rgb16.element =
      (uint16_t *)malloc(sizeof(uint16_t)*HUF->x3rgb16.size);
    break;
  case X3F_IMAGE_THUMB_HUFFMAN:
    HUF->rgb8.size = ID->columns * ID->rows * 3;
    HUF->rgb8.element =
      (uint8_t *)malloc(sizeof(uint8_t)*HUF->rgb8.size);
    break;
  default:
    fprintf(stderr, "Unknown huffman image type\n");
  }

  if (row_stride == 0)
    return x3f_load_huffman_compressed(I, DE, bits, use_map_table);
  else
    return x3f_load_huffman_not_compressed(I, DE, bits, use_map_table, row_stride);
}

static void x3f_load_pixmap(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  printf("Load pixmap\n");
  x3f_load_image_verbatim(I, DE);
}

static void x3f_load_jpeg(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  printf("Load JPEG\n");
  x3f_load_image_verbatim(I, DE);
}

static void x3f_load_image(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;

  read_data_set_offset(I, DE, X3F_IMAGE_HEADER_SIZE);

  switch (ID->type_format) {
  case X3F_IMAGE_RAW_TRUE:
  case X3F_IMAGE_RAW_MERRILL:
  case X3F_IMAGE_RAW_QUATTRO:
    x3f_load_true(I, DE);
    break;
  case X3F_IMAGE_RAW_HUFFMAN_X530:
  case X3F_IMAGE_RAW_HUFFMAN_10BIT:
    x3f_load_huffman(I, DE, 10, 1, ID->row_stride);
    break;
  case X3F_IMAGE_THUMB_PLAIN:
    x3f_load_pixmap(I, DE);
    break;
  case X3F_IMAGE_THUMB_HUFFMAN:
    x3f_load_huffman(I, DE, 8, 0, ID->row_stride);
    break;
  case X3F_IMAGE_THUMB_JPEG:
    x3f_load_jpeg(I, DE);
    break;
  default:
    fprintf(stderr, "Unknown image type\n");
  }
}

static void x3f_load_camf_decode_type2(x3f_camf_t *CAMF)
{
  uint32_t key = CAMF->t2.crypt_key;
  int i;

  CAMF->decoded_data_size = CAMF->data_size;
  CAMF->decoded_data = malloc(CAMF->decoded_data_size);

  for (i=0; i<CAMF->data_size; i++) {
    uint8_t old, new;
    uint32_t tmp;

    old = ((uint8_t *)CAMF->data)[i];
    key = (key * 1597 + 51749) % 244944;
    tmp = (uint32_t)(key * ((int64_t)301593171) >> 24);
    new = (uint8_t)(old ^ (uint8_t)(((((key << 8) - tmp) >> 1) + tmp) >> 17));
    ((uint8_t *)CAMF->decoded_data)[i] = new;
  }
}


/* NOTE: the unpacking in this code is in big respects identical to
   true_decode_one_color(). The difference is in the output you
   build. It might be possible to make some parts shared. NOTE ALSO:
   This means that the meta data is obfuscated using an image
   compression algorithm. */

static void camf_decode_type4(x3f_camf_t *CAMF)
{
  uint32_t seed = CAMF->t4.decode_bias;
  int row;

  uint8_t *dst;
  uint32_t dst_size = CAMF->t4.decoded_data_size;
  uint8_t *dst_end;

  bool_t odd_dst = 0;

  x3f_hufftree_t *tree = &CAMF->tree;
  bit_state_t BS;

  int32_t row_start_acc[2][2];
  uint32_t rows = CAMF->t4.block_count;
  uint32_t cols = CAMF->t4.block_size;

  CAMF->decoded_data_size = dst_size;

  CAMF->decoded_data = malloc(CAMF->decoded_data_size);
  memset(CAMF->decoded_data, 0, CAMF->decoded_data_size);

  dst = (uint8_t *)CAMF->decoded_data;
  dst_end = dst + dst_size;

  set_bit_state(&BS, CAMF->decoding_start);

  row_start_acc[0][0] = seed;
  row_start_acc[0][1] = seed;
  row_start_acc[1][0] = seed;
  row_start_acc[1][1] = seed;

  for (row = 0; row < rows; row++) {
    int col;
    bool_t odd_row = row&1;
    int32_t acc[2];

    /* We loop through all the columns and the rows. But the actual
       data is smaller than that, so we break the loop when reaching
       the end. */
    for (col = 0; col < cols; col++) {
      bool_t odd_col = col&1;
      int32_t diff = get_true_diff(&BS, tree);
      int32_t prev = col < 2 ?
	row_start_acc[odd_row][odd_col] :
	acc[odd_col];
      int32_t value = prev + diff;

      acc[odd_col] = value;
      if (col < 2)
	row_start_acc[odd_row][odd_col] = value;

      switch(odd_dst) {
      case 0:
	*dst++  = (uint8_t)((value>>4)&0xff);

	if (dst >= dst_end) {
	  goto ready;
	}

	*dst    = (uint8_t)((value<<4)&0xf0);
	break;
      case 1:
	*dst++ |= (uint8_t)((value>>8)&0x0f);

	if (dst >= dst_end) {
	  goto ready;
	}

	*dst++  = (uint8_t)((value<<0)&0xff);

	if (dst >= dst_end) {
	  goto ready;
	}

	break;
      }

      odd_dst = !odd_dst;
    } /* end col */
  } /* end row */

 ready:;
}

static void x3f_load_camf_decode_type4(x3f_camf_t *CAMF)
{
  int i;
  uint8_t *p;
  x3f_true_huffman_element_t *element = NULL;

  for (i=0, p = CAMF->data; *p != 0; i++) {
    /* TODO: Is this too expensive ??*/
    element =
      (x3f_true_huffman_element_t *)realloc(element, (i+1)*sizeof(*element));

    element[i].code_size = *p++;
    element[i].code = *p++;
  }

  CAMF->table.size = i;
  CAMF->table.element = element;

  /* TODO: where does the values 28 and 32 come from? */
#define CAMF_T4_DATA_SIZE_OFFSET 28
#define CAMF_T4_DATA_OFFSET 32
  CAMF->decoding_size = *(uint32_t *)(CAMF->data + CAMF_T4_DATA_SIZE_OFFSET);
  CAMF->decoding_start = (uint8_t *)CAMF->data + CAMF_T4_DATA_OFFSET;

  /* TODO: can it be fewer than 8 bits? Maybe taken from TRU->table? */
  new_huffman_tree(&CAMF->tree, 8);

  populate_true_huffman_tree(&CAMF->tree, &CAMF->table);

#ifdef DBG_PRNT
  print_huffman_tree(CAMF->tree.nodes, 0, 0);
#endif

  camf_decode_type4(CAMF);
}

static void camf_decode_type5(x3f_camf_t *CAMF)
{
  int32_t acc = CAMF->t5.decode_bias;

  uint8_t *dst;

  x3f_hufftree_t *tree = &CAMF->tree;
  bit_state_t BS;

  int32_t i;

  CAMF->decoded_data_size = CAMF->t5.decoded_data_size;
  CAMF->decoded_data = malloc(CAMF->decoded_data_size);

  dst = (uint8_t *)CAMF->decoded_data;

  set_bit_state(&BS, CAMF->decoding_start);

  for (i = 0; i < CAMF->decoded_data_size; i++) {
    int32_t diff = get_true_diff(&BS, tree);

    acc = acc + diff;
    *dst++ = (uint8_t)(acc & 0xff);
  }
}

static void x3f_load_camf_decode_type5(x3f_camf_t *CAMF)
{
  int i;
  uint8_t *p;
  x3f_true_huffman_element_t *element = NULL;

  for (i=0, p = CAMF->data; *p != 0; i++) {
    /* TODO: Is this too expensive ??*/
    element =
      (x3f_true_huffman_element_t *)realloc(element, (i+1)*sizeof(*element));

    element[i].code_size = *p++;
    element[i].code = *p++;
  }

  CAMF->table.size = i;
  CAMF->table.element = element;

  /* TODO: where does the values 28 and 32 come from? */
#define CAMF_T5_DATA_SIZE_OFFSET 28
#define CAMF_T5_DATA_OFFSET 32
  CAMF->decoding_size = *(uint32_t *)(CAMF->data + CAMF_T5_DATA_SIZE_OFFSET);
  CAMF->decoding_start = (uint8_t *)CAMF->data + CAMF_T5_DATA_OFFSET;

  /* TODO: can it be fewer than 8 bits? Maybe taken from TRU->table? */
  new_huffman_tree(&CAMF->tree, 8);

  populate_true_huffman_tree(&CAMF->tree, &CAMF->table);

#ifdef DBG_PRNT
  print_huffman_tree(CAMF->tree.nodes, 0, 0);
#endif

  camf_decode_type5(CAMF);
}

static void x3f_setup_camf_text_entry(camf_entry_t *entry)
{
  entry->text_size = *(uint32_t *)entry->value_address;
  entry->text = entry->value_address + 4;
}

static void x3f_setup_camf_property_entry(camf_entry_t *entry)
{
  int i;
  uint8_t *e =
    entry->entry;
  uint8_t *v =
    entry->value_address;
  uint32_t num =
    entry->property_num = *(uint32_t *)v;
  uint32_t off = *(uint32_t *)(v + 4);

  entry->property_name = (char **)malloc(num*sizeof(uint8_t*));
  entry->property_value = (uint8_t **)malloc(num*sizeof(uint8_t*));

  for (i=0; i<num; i++) {
    uint32_t name_off = off + *(uint32_t *)(v + 8 + 8*i);
    uint32_t value_off = off + *(uint32_t *)(v + 8 + 8*i + 4);

    entry->property_name[i] = (char *)(e + name_off);
    entry->property_value[i] = e + value_off;
  }
}

static void set_matrix_element_info(uint32_t type,
				    uint32_t *size,
				    matrix_type_t *decoded_type)
{
  switch (type) {
  case 0:
    *size = 2;
    *decoded_type = M_INT; /* known to be true */
    break;
  case 1:
    *size = 4;
    *decoded_type = M_UINT; /* TODO: unknown ???? */
    break;
  case 2:
    *size = 4;
    *decoded_type = M_UINT; /* TODO: unknown ???? */
    break;
  case 3:
    *size = 4;
    *decoded_type = M_FLOAT; /* known to be true */
    break;
  case 5:
    *size = 1;
    *decoded_type = M_UINT; /* TODO: unknown ???? */
    break;
  case 6:
    *size = 2;
    *decoded_type = M_UINT; /* TODO: unknown ???? */
    break;
  default:
    fprintf(stderr, "Unknown matrix type (%ud)\n", type);
    assert(0);
  }
}

static void get_matrix_copy(camf_entry_t *entry)
{
  uint32_t element_size = entry->matrix_element_size;
  uint32_t elements = entry->matrix_elements;
  int i, size = (entry->matrix_decoded_type==M_FLOAT ?
		 sizeof(double) :
		 sizeof(uint32_t)) * elements;

  entry->matrix_decoded = malloc(size);

  switch (element_size) {
  case 4:
    switch (entry->matrix_decoded_type) {
    case M_INT:
    case M_UINT:
      memcpy(entry->matrix_decoded, entry->matrix_data, size);
      break;
    case M_FLOAT:
      for (i=0; i<elements; i++)
	((double *)entry->matrix_decoded)[i] =
	  (double)((float *)entry->matrix_data)[i];
      break;
    default:
      fprintf(stderr, "Invalid matrix element type of size 4\n");
      assert(0);
    }
    break;
  case 2:
    switch (entry->matrix_decoded_type) {
    case M_INT:
      for (i=0; i<elements; i++)
	((int32_t *)entry->matrix_decoded)[i] =
	  (int32_t)((int16_t *)entry->matrix_data)[i];
      break;
    case M_UINT:
      for (i=0; i<elements; i++)
	((uint32_t *)entry->matrix_decoded)[i] =
	  (uint32_t)((uint16_t *)entry->matrix_data)[i];
      break;
    default:
      fprintf(stderr, "Invalid matrix element type of size 2\n");
      assert(0);
    }
    break;
  case 1:
    switch (entry->matrix_decoded_type) {
    case M_INT:
      for (i=0; i<elements; i++)
	((int32_t *)entry->matrix_decoded)[i] =
	  (int32_t)((int8_t *)entry->matrix_data)[i];
      break;
    case M_UINT:
      for (i=0; i<elements; i++)
	((uint32_t *)entry->matrix_decoded)[i] =
	  (uint32_t)((uint8_t *)entry->matrix_data)[i];
      break;
    default:
      fprintf(stderr, "Invalid matrix element type of size 1\n");
      assert(0);
    }
    break;
  default:
    fprintf(stderr, "Unknown size %d\n", element_size);
    assert(0);
  }
}

static void x3f_setup_camf_matrix_entry(camf_entry_t *entry)
{
  int i;
  int totalsize = 1;

  uint8_t *e =
    entry->entry;
  uint8_t *v =
    entry->value_address;
  uint32_t type =
    entry->matrix_type = *(uint32_t *)(v + 0);
  uint32_t dim =
    entry->matrix_dim = *(uint32_t *)(v + 4);
  uint32_t off =
    entry->matrix_data_off = *(uint32_t *)(v + 8);
  camf_dim_entry_t *dentry =
    entry->matrix_dim_entry =
    (camf_dim_entry_t*)malloc(dim*sizeof(camf_dim_entry_t));

  for (i=0; i<dim; i++) {
    uint32_t size =
      dentry[i].size = *(uint32_t *)(v + 12 + 12*i + 0);
    dentry[i].name_offset = *(uint32_t *)(v + 12 + 12*i + 4);
    dentry[i].n = *(uint32_t *)(v + 12 + 12*i + 8);
    dentry[i].name = (char *)(e + dentry[i].name_offset);

    if (dentry[i].n != i) {
      /* TODO: is something needed to be made in this case */
      fprintf(stderr,
	      "WARNING: matrix entry for %s/%s is out of order "
	      "(index/%d != order/%d)\n",
	      entry->name_address, dentry[i].name, dentry[i].n, i);
    }

    totalsize *= size;
  }

  set_matrix_element_info(type,
			  &entry->matrix_element_size,
			  &entry->matrix_decoded_type);
  entry->matrix_data = (void *)(e + off);

  entry->matrix_elements = totalsize;
  entry->matrix_used_space = entry->entry_size - off;

  /* This estimate only works for matrices above a certain size */
  entry->matrix_estimated_element_size = entry->matrix_used_space / totalsize;

  get_matrix_copy(entry);
}

static void x3f_setup_camf_entries(x3f_camf_t *CAMF)
{
  uint8_t *p = (uint8_t *)CAMF->decoded_data;
  uint8_t *end = p + CAMF->decoded_data_size;
  camf_entry_t *entry = NULL;
  int i;

  printf("SETUP CAMF ENTRIES\n");

  for (i=0; p < end; i++) {
    uint32_t *p4 = (uint32_t *)p;

    switch (*p4) {
    case X3F_CMbP:
    case X3F_CMbT:
    case X3F_CMbM:
      break;
    default:
      fprintf(stderr, "Unknown CAMF entry %x @ %p\n", *p4, p4);
      fprintf(stderr, "  start = %p end = %p\n", CAMF->decoded_data, end);
      fprintf(stderr, "  left = %ld\n", (long)(end - p));
      fprintf(stderr, "Stop parsing CAMF\n");
      goto stop;
    }

    /* TODO: lots of realloc - may be inefficient */
    entry = (camf_entry_t *)realloc(entry, (i+1)*sizeof(camf_entry_t));

    /* Pointer */
    entry[i].entry = p;

    /* Header */
    entry[i].id = *p4++;
    entry[i].version = *p4++;
    entry[i].entry_size = *p4++;
    entry[i].name_offset = *p4++;
    entry[i].value_offset = *p4++;

    /* Compute adresses and sizes */
    entry[i].name_address = (char *)(p + entry[i].name_offset);
    entry[i].value_address = p + entry[i].value_offset;
    entry[i].name_size = entry[i].value_offset - entry[i].name_offset;
    entry[i].value_size = entry[i].entry_size - entry[i].value_offset;

    entry[i].text_size = 0;
    entry[i].text = NULL;
    entry[i].property_num = 0;
    entry[i].property_name = NULL;
    entry[i].property_value = NULL;
    entry[i].matrix_type = 0;
    entry[i].matrix_dim = 0;
    entry[i].matrix_data_off = 0;
    entry[i].matrix_data = NULL;
    entry[i].matrix_dim_entry = NULL;

    entry[i].matrix_decoded = NULL;

    switch (entry[i].id) {
    case X3F_CMbP:
      x3f_setup_camf_property_entry(&entry[i]);
      break;
    case X3F_CMbT:
      x3f_setup_camf_text_entry(&entry[i]);
      break;
    case X3F_CMbM:
      x3f_setup_camf_matrix_entry(&entry[i]);
      break;
    }

    p += entry[i].entry_size;
  }

 stop:

  CAMF->entry_table.size = i;
  CAMF->entry_table.element = entry;

  printf("SETUP CAMF ENTRIES (READY) Found %d entries\n", i);
}

static void x3f_load_camf(x3f_info_t *I, x3f_directory_entry_t *DE)
{
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_camf_t *CAMF = &DEH->data_subsection.camf;

  printf("Loading CAMF of type %d\n", CAMF->type);

  read_data_set_offset(I, DE, X3F_CAMF_HEADER_SIZE);

  CAMF->data_size = read_data_block(&CAMF->data, I, DE, 0);

  switch (CAMF->type) {
  case 2:			/* Older SD9-SD14 */
    x3f_load_camf_decode_type2(CAMF);
    break; 
  case 4:			/* TRUE ... Merrill */
    x3f_load_camf_decode_type4(CAMF);
    break;
  case 5:			/* Quattro ... */
    x3f_load_camf_decode_type5(CAMF);
    break;
  default:
    fprintf(stderr, "Unknown CAMF type\n");
  }

  if (CAMF->decoded_data != NULL)
    x3f_setup_camf_entries(CAMF);
  else
    fprintf(stderr, "No decoded CAMF data\n");
}

/* extern */ x3f_return_t x3f_load_data(x3f_t *x3f, x3f_directory_entry_t *DE)
{
  x3f_info_t *I = &x3f->info;

  if (DE == NULL)
    return X3F_ARGUMENT_ERROR;

  switch (DE->header.identifier) {
  case X3F_SECp:
    x3f_load_property_list(I, DE);
    break;
  case X3F_SECi:
    x3f_load_image(I, DE);
    break;
  case X3F_SECc:
    x3f_load_camf(I, DE);
    break;
  default:
    fprintf(stderr, "Unknown directory entry type\n");
    return X3F_INTERNAL_ERROR;
  }

  return X3F_OK;
}

/* extern */ x3f_return_t x3f_load_image_block(x3f_t *x3f, x3f_directory_entry_t *DE)
{
  x3f_info_t *I = &x3f->info;

  if (DE == NULL)
    return X3F_ARGUMENT_ERROR;

  printf("Load image block\n");

  switch (DE->header.identifier) {
  case X3F_SECi:
    read_data_set_offset(I, DE, X3F_IMAGE_HEADER_SIZE);
    x3f_load_image_verbatim(I, DE);
    break;
  default:
    fprintf(stderr, "Unknown image directory entry type\n");
    return X3F_INTERNAL_ERROR;
  }

  return X3F_OK;
}

static int get_camf_matrix_var(x3f_t *x3f, char *name,
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
    fprintf(stderr, "Could not get entry %s: CAMF section not found\n", name);
    return 0;
  }

  DEH = &DE->header;
  CAMF = &DEH->data_subsection.camf;
  table = CAMF->entry_table.element;

  for (i=0; i<CAMF->entry_table.size; i++) {
    camf_entry_t *entry = &table[i];

    if (!strcmp(name, entry->name_address)) {
      if (entry->id != X3F_CMbM) {
	fprintf(stderr, "CAMF entry is not a matrix: %s\n", name);
	return 0;
      }
      if (entry->matrix_decoded_type != type) {
	fprintf(stderr, "CAMF entry not required type: %s\n", name);
	return 0;
      }

      switch (entry->matrix_dim) {
      case 3:
	if (dim2 == NULL || dim1 == NULL || dim0 == NULL) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim2 = entry->matrix_dim_entry[2].size;
	*dim1 = entry->matrix_dim_entry[1].size;
	*dim0 = entry->matrix_dim_entry[0].size;
      break;
      case 2:
	if (dim2 != NULL || dim1 == NULL || dim0 == NULL) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim1 = entry->matrix_dim_entry[1].size;
	*dim0 = entry->matrix_dim_entry[0].size;
      break;
      case 1:
	if (dim2 != NULL || dim1 != NULL || dim0 == NULL) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	*dim0 = entry->matrix_dim_entry[0].size;
	break;
      default:
	fprintf(stderr, "CAMF entry - more than 3 dimensions: %s\n", name);
	return 0;
      }

      printf("Getting CAMF matrix for %s\n", name);
      *matrix = entry->matrix_decoded;
      return 1;
    }
  }

  fprintf(stderr, "CAMF entry not found: %s\n", name);

  return 0;
}

static int get_camf_matrix(x3f_t *x3f, char *name,
			   int dim0, int dim1, int dim2, matrix_type_t type,
			   void *matrix)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    fprintf(stderr, "Could not get entry %s: CAMF section not found\n", name);
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
	fprintf(stderr, "CAMF entry is not a matrix: %s\n", name);
	return 0;
      }
      if (entry->matrix_decoded_type != type) {
	fprintf(stderr, "CAMF entry not required type: %s\n", name);
	return 0;
      }

      switch (entry->matrix_dim) {
      case 3:
	if (dim2 != entry->matrix_dim_entry[2].size ||
	    dim1 != entry->matrix_dim_entry[1].size ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      case 2:
	if (dim2 != 0 ||
	    dim1 != entry->matrix_dim_entry[1].size ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      case 1:
	if (dim2 != 0 ||
	    dim1 != 0 ||
	    dim0 != entry->matrix_dim_entry[0].size) {
	  fprintf(stderr, "CAMF entry - wrong dimension size: %s\n", name);
	  return 0;
	}
	break;
      default:
	fprintf(stderr, "CAMF entry - more than 3 dimensions: %s\n", name);
	return 0;
      }

      size = (entry->matrix_decoded_type==M_FLOAT ?
	      sizeof(double) :
	      sizeof(uint32_t)) * entry->matrix_elements;
      printf("Copying CAMF matrix for %s\n", name);
      memcpy(matrix, entry->matrix_decoded, size);
      return 1;
    }
  }

  fprintf(stderr, "CAMF entry not found: %s\n", name);

  return 0;
}

static int get_camf_float(x3f_t *x3f, char *name,  double *val)
{
  return get_camf_matrix(x3f, name, 1, 0, 0, M_FLOAT, val);
}

static int get_camf_float_vector(x3f_t *x3f, char *name,  double *val)
{
  return get_camf_matrix(x3f, name, 3, 0, 0, M_FLOAT, val);
}

static int get_camf_unsigned(x3f_t *x3f, char *name,  uint32_t *val)
{
  return get_camf_matrix(x3f, name, 1, 0, 0, M_UINT, val);
}

static int get_camf_signed(x3f_t *x3f, char *name,  int32_t *val)
{
  return get_camf_matrix(x3f, name, 1, 0, 0, M_INT, val);
}

static int get_camf_signed_vector(x3f_t *x3f, char *name,  int32_t *val)
{
  return get_camf_matrix(x3f, name, 3, 0, 0, M_INT, val);
}

static int get_camf_property_list(x3f_t *x3f, char *list,
				  char ***names, char ***values, uint32_t *num)
{
  x3f_directory_entry_t *DE = x3f_get_camf(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_camf_t *CAMF;
  camf_entry_t *table;
  int i;

  if (!DE) {
    fprintf(stderr, "Could not get entry %s: CAMF section not found\n",
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
	fprintf(stderr, "CAMF entry is not a property list: %s\n", list);
	return 0;
      }

      printf("Getting CAMF property list for %s\n", list);
      *names = entry->property_name;
      *values = (char **)entry->property_value;
      *num = entry->property_num;
      return 1;
    }
  }

  fprintf(stderr, "CAMF entry not found: %s\n", list);
  
  return 0;
}

static int get_camf_property(x3f_t *x3f, char *list, char *name, char **value)
{
  char **names, **values;
  uint32_t num, i;

  if (!get_camf_property_list(x3f, list, &names, &values, &num))
    return 0;
  
  for (i=0; i<num; i++)
    if (!strcmp(names[i], name)) {
	*value = values[i];
	return 1;
    }

  fprintf(stderr, "CAMF poperty '%s' not found in list '%s'\n", name, list);

  return 0;
}

static int get_prop_entry(x3f_t *x3f, char *name, char **value)
{
  x3f_directory_entry_t *DE = x3f_get_prop(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_property_list_t *PL;
  x3f_property_t *table;
  int i;

  if (!DE) {
    fprintf(stderr, "Could not get property %s: PROP section not found\n",
	    name);
    return 0;
  }

  DEH = &DE->header;
  PL = &DEH->data_subsection.property_list;
  table = PL->property_table.element;

  for (i=0; i<PL->property_table.size; i++) {
    x3f_property_t *entry = &table[i];

    if (!strcmp(name, entry->name_utf8)) {
      printf("Getting PROP entry \"%s\" = \"%s\"\n", name, entry->value_utf8);
      *value = entry->value_utf8;
      return 1;
    }
  }

  fprintf(stderr, "PROP entry not found: %s\n", name);

  return 0;
}

static char *get_wb(x3f_t *x3f)
{
  uint32_t wb;

  if (get_camf_unsigned(x3f, "WhiteBalance", &wb)) {
    /* Quattro. TODO: any better way to do this? Maybe get the info
       from the EXIF in the JPEG? */
    switch (wb) {
    case 1:  return "Auto";
    case 2:  return "Sunlight";
    case 3:  return "Shadow";
    case 4:  return "Overcast";
    case 5:  return "Incandenscent";
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

static int get_camf_matrix_for_wb(x3f_t *x3f,
				  char *list, char *wb, int dim0, int dim1,
				  double *matrix)
{
  char *matrix_name;

  if (!get_camf_property(x3f, list, wb, &matrix_name)) {
    if (!strcmp(wb, "Daylight")) /* Workaround for bug in SD1 */
      return get_camf_matrix_for_wb(x3f, list, "Sunlight", dim0, dim1, matrix);
    return 0;
  }

  return get_camf_matrix(x3f, matrix_name, dim0, dim1, 0, M_FLOAT, matrix);
}

static int is_TRUE_engine(x3f_t *x3f)
{
  char **names, **values;
  uint32_t num;

  /* TODO: is there a better way to test this */
  if ((get_camf_property_list(x3f, "WhiteBalanceColorCorrections",
			      &names, &values, &num) ||
       get_camf_property_list(x3f, "DP1_WhiteBalanceColorCorrections",
			      &names, &values, &num)) &&
      (get_camf_property_list(x3f, "WhiteBalanceGains",
			      &names, &values, &num) ||
       get_camf_property_list(x3f, "DP1_WhiteBalanceGains",
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

static int get_max_raw(x3f_t *x3f, uint32_t *max_raw)
{
  uint32_t image_depth;

  if (get_camf_unsigned(x3f, "ImageDepth", &image_depth)) {
    uint32_t max = (1<<image_depth) - 1;
    max_raw[0] = max;
    max_raw[1] = max;
    max_raw[2] = max;
    return 1;
  }

  /* It seems that RawSaturationLevel should be used for TURE engine and
     SaturationLevel for pre-TRUE engine */
  if (is_TRUE_engine(x3f))
    return get_camf_signed_vector(x3f, "RawSaturationLevel",
				  (int32_t *)max_raw);
  else
    return get_camf_signed_vector(x3f, "SaturationLevel",
				  (int32_t *)max_raw);
}

static int image_area(x3f_t *x3f, x3f_area_t *image)
{
  x3f_directory_entry_t *DE = x3f_get_raw(x3f);
  x3f_directory_entry_header_t *DEH;
  x3f_image_data_t *ID;
  x3f_huffman_t *HUF;
  x3f_true_t *TRU;
  uint16_t *data = NULL;

  if (!DE) return 0;

  DEH = &DE->header;
  ID = &DEH->data_subsection.image_data;
  HUF = ID->huffman;
  TRU = ID->tru;

  if (HUF != NULL)
    data = HUF->x3rgb16.element;

  if (TRU != NULL)
    data = TRU->x3rgb16.element;

  if (!data) return 0;

  image->data = data;
  image->columns = ID->columns;
  image->rows = ID->rows;
  image->channels = 3;
  image->row_stride = ID->columns * image->channels;

  return 1;
}

static int crop_area(uint32_t *coord, x3f_area_t *image, x3f_area_t *crop)
{
  if (coord[0] > coord[2] || coord[1] > coord[3]) return 0;
  if (coord[2] >= image->columns || coord[3] >= image->rows) return 0;

  crop->data =
    image->data + image->channels*coord[0] + image->row_stride*coord[1];
  crop->columns = coord[2] - coord[0] + 1;
  crop->rows = coord[3] - coord[1] + 1;
  crop->channels = image->channels;
  crop->row_stride = image->row_stride;

  return 1;
}

static int get_camf_rect(x3f_t *x3f, char *name, uint32_t *rect)
{
  uint32_t keep[4];

 if (!get_camf_matrix(x3f, name, 4, 0, 0, M_UINT, rect)) return 0;
 if (!get_camf_matrix(x3f, "KeepImageArea", 4, 0, 0, M_UINT, keep)) return 0;

 /* KeepImageArea is the part of the sensor output that gets stored to
    X3F. Coordinates in CAMF are relative to the actual sensor, NOT
    the X3F image data. */

 if (rect[0] > keep[2] || rect[1] > keep[3] ||
     rect[2] < keep[0] || rect[3] < keep[1]) {
   fprintf(stderr,
	   "WARNING: CAMF rect %s (%u,%u,%u,%u) completely outside "
	   "KeepImageArea (%u,%u,%u,%u)\n", name,
	   rect[0], rect[1], rect[2], rect[3],
	   keep[0], keep[1], keep[2], keep[3]);
   return 0;
 }

 if (rect[0] < keep[0]) rect[0] = keep[0];
 if (rect[1] < keep[1]) rect[1] = keep[1];
 if (rect[2] > keep[2]) rect[2] = keep[2];
 if (rect[3] > keep[3]) rect[3] = keep[3];
 
 rect[0] -= keep[0];
 rect[1] -= keep[1];
 rect[2] -= keep[0];
 rect[3] -= keep[1];
   
 return 1;
}

static int crop_area_camf(x3f_t *x3f, char *name,
			  x3f_area_t *image, x3f_area_t *crop)
{
  uint32_t coord[4];

  if (!get_camf_rect(x3f, name, coord)) return 0;
  if (!crop_area(coord, image, crop)) {
    fprintf(stderr,
	    "WARNING: CAMF crop out of bounds: %s (%u,%u,%u,%u) : (%u,%u)\n",
	    name, coord[0], coord[1], coord[2], coord[3],
	    image->columns, image->rows);
    return 0;
  }

  return 1;
}

static int sum_area(x3f_t *x3f, char *name, uint64_t *sum /* in/out */)
{
  x3f_area_t image, area;
  int row, col, color;

  if (!image_area(x3f, &image) || image.channels < 3) return 0;
  if (!crop_area_camf(x3f, name, &image, &area)) return 0;

  for (row = 0; row < area.rows; row++)
    for (col = 0; col < area.columns; col++)
      for (color = 0; color < 3; color++)
	sum[color] += (uint64_t)area.data[area.row_stride*row +
					  area.channels*col + color];

  return area.columns*area.rows;
}

static void get_black_level(x3f_t *x3f, double *black_level)
{
  uint64_t black_sum[3] = {0,0,0};
  int black_pixels = 0;
  int i;
  
  black_pixels += sum_area(x3f, "DarkShieldTop", black_sum);
  black_pixels += sum_area(x3f, "DarkShieldBottom", black_sum);

  if (black_pixels == 0) {
    black_level[0] = black_level[1] = black_level[2] = 0.0;
    fprintf(stderr,
	    "WARNING: could not calculate black level, assuming {%g,%g,%g}\n",
	    black_level[0], black_level[1], black_level[2]);
    return;
  }

  for (i=0; i<3; i++)
    black_level[i] = (double)black_sum[i]/black_pixels;
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

  if (get_camf_matrix_for_wb(x3f, "WhiteBalanceGains", wb, 3, 0, gain) ||
      get_camf_matrix_for_wb(x3f, "DP1_WhiteBalanceGains", wb, 3, 0, gain));
  else if (get_camf_matrix_for_wb(x3f, "WhiteBalanceIlluminants", wb,
				  3, 3, cam_to_xyz) &&
	   get_camf_matrix_for_wb(x3f, "WhiteBalanceCorrections", wb,
				  3, 3, wb_correction)) {
    double raw_to_xyz[9], raw_neutral[3];

    x3f_3x3_3x3_mul(wb_correction, cam_to_xyz, raw_to_xyz);
    get_raw_neutral(raw_to_xyz, raw_neutral);
    x3f_3x1_invert(raw_neutral, gain);
  }
  else
    return 0;

  if (get_camf_float_vector(x3f, "SensorAdjustmentGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);

  if (get_camf_float_vector(x3f, "TempGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);
     
  if (get_camf_float_vector(x3f, "FNumberGainFact", gain_fact))
    x3f_3x1_comp_mul(gain_fact, gain, gain);

  printf("gain\n");
  x3f_3x1_print(gain);

  return 1;
}

static int get_bmt_to_xyz(x3f_t *x3f, char *wb, double *bmt_to_xyz)
{
  double cc_matrix[9], cam_to_xyz[9], wb_correction[9];
  
  if (get_camf_matrix_for_wb(x3f, "WhiteBalanceColorCorrections", wb,
			     3, 3, cc_matrix) ||
      get_camf_matrix_for_wb(x3f, "DP1_WhiteBalanceColorCorrections", wb,
			     3, 3, cc_matrix)) {
    double srgb_to_xyz[9];

    x3f_sRGB_to_XYZ(srgb_to_xyz);
    x3f_3x3_3x3_mul(srgb_to_xyz, cc_matrix, bmt_to_xyz);
  }
  else if (get_camf_matrix_for_wb(x3f, "WhiteBalanceIlluminants", wb,
				  3, 3, cam_to_xyz) &&
	   get_camf_matrix_for_wb(x3f, "WhiteBalanceCorrections", wb,
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

static double lens_position(double focal_length, double object_distance)
{
  return 1.0/(1.0/focal_length - 1.0/object_distance);
}

static double get_focal_length(x3f_t *x3f)
{
  char *flength;
  double focal_length;

  if (get_prop_entry(x3f, "FLENGTH", &flength))
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

  if (get_camf_float(x3f, "ObjectDistance", &object_distance))
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

  if (!get_camf_signed(x3f, "LensInformation", &lens_information))
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

  spatial_gain_corr_merrill_t mgain[4]; /* raw merril type gains */
  int mgain_num;
} spatial_gain_corr_t;

static int get_merril_type_gains_table(x3f_t *x3f, char *name, char *chan,
				       uint32_t **mgain, int *rows, int *cols,
				       double *mingain, double *delta)
{
  char table[32], *val;
  int rows_tmp, cols_tmp;

  sprintf(table, "GainsTable%s", chan);
  if (!get_camf_property(x3f, name, table, &val) ||
      !get_camf_matrix_var(x3f, val, &rows_tmp, &cols_tmp, NULL,
			   M_UINT, (void **)mgain) ||
      (*rows != -1 && *rows != rows_tmp) ||
      (*cols != -1 && *cols != cols_tmp))
    return 0;
  *rows = rows_tmp;
  *cols = cols_tmp;

  sprintf(table, "MinGains%s", chan);
  if (!get_camf_property(x3f, name, table, &val)) return 0;
  *mingain = atof(val);

  sprintf(table, "Delta%s", chan);
  if (!get_camf_property(x3f, name, table, &val)) return 0;
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

  if (!get_camf_float(x3f, "CaptureAperture", &capture_aperture)) return 0;

  if (!get_camf_property_list(x3f, "IncludeBlocks",
			      &include_blocks, &include_blocks_val,
			      &include_blocks_num))
    return 0;

  if (hp_flag) {		/* Quattro HP */
    if (!get_camf_matrix_var(x3f, "SpatialGainHP_Fstop", &num_fstop, NULL, NULL,
			     M_FLOAT, (void **)&spatial_gain_fstop))
      return 0;
    corr_num = 6; /* R, G, B0, B1, B2, B3 */
    
    for (i=0; i<include_blocks_num; i++) {
      char **names, **values;
      uint32_t num, aperture_index;
      char dummy;
      
      if (sscanf(include_blocks[i], "SpatialGainHPProps_%d%c",
		 &aperture_index, &dummy) == 1 &&
	  get_camf_property_list(x3f, include_blocks[i],
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
    if (get_camf_matrix_var(x3f, "SpatialGain_Fstop", &num_fstop, NULL, NULL,
			    M_FLOAT, (void **)&spatial_gain_fstop))
      for (i=0; i<include_blocks_num; i++) {
	char **names, **values;
	uint32_t num, aperture_index;
	char focus_distance[4], dummy;
	
	if (sscanf(include_blocks[i], "SpatialGainsProps_%d_%3s%c",
		   &aperture_index, focus_distance, &dummy) == 2 &&
	    get_camf_property_list(x3f, include_blocks[i],
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
	    get_camf_property_list(x3f, include_blocks[i],
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
      if (!get_merril_type_gains_table(x3f, name, channels[j],
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
  
  if ((get_camf_property(x3f, "SpatialGainTables", wb, &gain_name) &&
       get_camf_matrix_var(x3f, gain_name,
			   &corr->rows, &corr->cols, &corr->channels, M_FLOAT,
			   (void **)&corr->gain)) ||
      get_camf_matrix_var(x3f, "SpatialGain",
			  &corr->rows, &corr->cols, &corr->channels, M_FLOAT,
			  (void **)&corr->gain)) {
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

/* x3f_denoise expects a 14-bit image. Using 13 bits to avoid clipping
   when 2.0 >= gain > 1.0 */
#define INTERMEDIATE_DEPTH 13
#define INTERMEDIATE_UNIT ((1<<INTERMEDIATE_DEPTH) - 1)

static int preprocess_data(x3f_t *x3f, x3f_area_t *image, char *wb)
{
  int row, col, color;
  uint32_t max_raw[3];
  double gain[3], scale[3], black_level[3];
  spatial_gain_corr_t sgain[MAXCORR];
  int sgain_num;
  
  if (image->channels < 3) return 0;
  
  if (!get_max_raw(x3f, max_raw)) {
    fprintf(stderr, "Could not get maximum RAW level\n");
    return 0;
  }
  printf("max_raw = {%d,%d,%d}\n", max_raw[0], max_raw[1], max_raw[2]);
  
  get_black_level(x3f, black_level);
  printf("black_level = {%g,%g,%g}\n",
	 black_level[0], black_level[1], black_level[2]);

  if (!get_gain(x3f, wb, gain)) {
    fprintf(stderr, "Could not get gain for white balance: %s\n", wb);
    return 0;
  }

  for (color = 0; color < 3; color++)
    scale[color] = INTERMEDIATE_UNIT*gain[color] / 
      (max_raw[color] - black_level[color]);
  
  sgain_num = get_spatial_gain(x3f, wb, sgain);
  if (sgain_num == 0)
    fprintf(stderr, "WARNING: could not get spatial gain\n");
  
  for (row = 0; row < image->rows; row++)
    for (col = 0; col < image->columns; col++)
      for (color = 0; color < 3; color++) {
	uint16_t *valp =
	  &image->data[image->row_stride*row + image->channels*col + color];
	int32_t out =
	  (int32_t)((*valp - black_level[color]) * scale[color] *
		    calc_spatial_gain(sgain, sgain_num,
				      row, col, color,
				      image->rows, image->columns));
	if (out < 0) *valp = 0;
	else if (out > 65535) *valp = 65535;
	else *valp = out;
      }

  cleanup_spatial_gain(sgain, sgain_num);

  return 1;
}

/* Converts the data in place */

#define LUTSIZE 1024

static int convert_data(x3f_t *x3f, x3f_area_t *image,
			x3f_color_encoding_t encoding, char *wb)
{
  int row, col, color;
  uint16_t max_out = 65535; /* TODO: should be possible to adjust */

  double bmt_to_xyz[9];	/* White point for XYZ is assumed to be D65 */
  double xyz_to_rgb[9];
  double bmt_to_rgb[9];
  double conv_matrix[9];
  double sensor_iso, capture_iso, iso_scaling;
  double lut[LUTSIZE];

  if (wb == NULL) wb = get_wb(x3f);

  if (image->channels < 3) return X3F_ARGUMENT_ERROR;

  if (get_camf_float(x3f, "SensorISO", &sensor_iso) &&
      get_camf_float(x3f, "CaptureISO", &capture_iso)) {
    printf("SensorISO = %g\n", sensor_iso);
    printf("CaptureISO = %g\n", capture_iso);
    iso_scaling = capture_iso/sensor_iso;
  }
  else {
    iso_scaling = 1.0;
    fprintf(stderr, "WARNING: could not calculate ISO scaling, assuming %g\n",
	    iso_scaling);
  }

  if (!get_bmt_to_xyz(x3f, wb, bmt_to_xyz)) {
    fprintf(stderr, "Could not get bmt_to_xyz for white balance: %s\n", wb);
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

  x3f_3x3_3x3_mul(xyz_to_rgb, bmt_to_xyz, bmt_to_rgb);
  x3f_scalar_3x3_mul(iso_scaling, bmt_to_rgb, conv_matrix);

  printf("bmt_to_rgb\n");
  x3f_3x3_print(bmt_to_rgb);
  printf("conv_matrix\n");
  x3f_3x3_print(conv_matrix);

  for (row = 0; row < image->rows; row++) {
    for (col = 0; col < image->columns; col++) {
      uint16_t *valp[3];
      double input[3], output[3];

      /* Get the data */
      for (color = 0; color < 3; color++) {
	valp[color] = 
	  &image->data[image->row_stride*row + image->channels*col + color];
	input[color] = (double)*valp[color]/INTERMEDIATE_UNIT;
      }

      /* Do color conversion */
      x3f_3x3_3x1_mul(conv_matrix, input, output);

      /* Write back the data, doing non linear coding */
      for (color = 0; color < 3; color++)
	*valp[color] = x3f_LUT_lookup(lut, LUTSIZE, output[color]);
    }
  }

  return 1;
}

static int run_denoising(x3f_t *x3f)
{
  x3f_area_t original_image, image;
  x3f_denoise_type_t type = X3F_DENOISE_STD;
  char *sensorid;

  if (!image_area(x3f, &original_image)) return 0;
  if (!crop_area_camf(x3f, "ActiveImageArea", &original_image, &image)) {
    image = original_image;
    fprintf(stderr,
	    "WARNING: could not get active area, denoising entire image\n");
  }

  if (get_prop_entry(x3f, "SENSORID", &sensorid) && !strcmp(sensorid, "F20"))
    type = X3F_DENOISE_F20;

  x3f_denoise(&image, type);
  return 1;
}

static x3f_return_t get_image(x3f_t *x3f,
			      x3f_area_t *image,
			      x3f_color_encoding_t encoding,
			      int crop,
			      int denoise,
			      char *wb)
{
  x3f_area_t original_image;

  if (!image_area(x3f, &original_image)) return X3F_ARGUMENT_ERROR;
  if (!preprocess_data(x3f, &original_image, wb)) return X3F_ARGUMENT_ERROR;
  if (denoise && !run_denoising(x3f)) return X3F_ARGUMENT_ERROR;

  if (!crop || !crop_area_camf(x3f, "ActiveImageArea", &original_image, image))
    *image = original_image;

  if (encoding != NONE && !convert_data(x3f, image, encoding, wb))
      return X3F_ARGUMENT_ERROR;

  return X3F_OK;
}

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
  x3f_return_t ret;
  x3f_area_t image;
  FILE *f_out = fopen(outfilename, "wb");
  int row;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  ret = get_image(x3f, &image, encoding, crop, denoise, wb);
  if (ret != X3F_OK) {
    fclose(f_out);
    return ret;
  }
  if (image.channels < 3) {
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
  x3f_return_t ret;
  x3f_area_t image;
  TIFF *f_out = TIFFOpen(outfilename, "w");
  int row;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  ret = get_image(x3f, &image, encoding, crop, denoise, wb);
  if (ret != X3F_OK) {
    TIFFClose(f_out);
    return ret;
  }

  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, image.columns);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, image.rows);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, 32);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, image.channels);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(f_out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(f_out, TIFFTAG_XRESOLUTION, 72.0);
  TIFFSetField(f_out, TIFFTAG_YRESOLUTION, 72.0);
  TIFFSetField(f_out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

  for (row=0; row < image.rows; row++)
    TIFFWriteScanline(f_out, image.data + image.row_stride*row, row, 0);

  TIFFWriteDirectory(f_out);
  TIFFClose(f_out);

  return X3F_OK;
}

static void vec_double_to_float(double *a, float *b, int len)
{
  int i;

  for (i=0; i<len; i++)
    b[i] = a[i];
}

#if 0
static int write_camera_profile_for_wb(x3f_t *x3f, TIFF *tiff, char *wb)
{
  double raw_to_xyz[9], xyz_to_raw[9];
  float color_matrix1[9];

  if (!get_raw_to_xyz_for_wb(x3f, wb, raw_to_xyz)) return 0;
  x3f_3x3_inverse(raw_to_xyz, xyz_to_raw);

  printf("xyz_to_raw\n");
  x3f_3x3_print(xyz_to_raw);

  vec_double_to_float(xyz_to_raw, color_matrix1, 9);
  TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, color_matrix1);
  TIFFSetField(tiff, TIFFTAG_PROFILENAME, wb);
  /* Tell the raw converter to refrian from clipping the dark areas */
  TIFFSetField(tiff, TIFFTAG_DEFAULTBLACKRENDER, 1);

  return 1;
}

static x3f_return_t write_camera_profiles(x3f_t *x3f, TIFF *tiff)
{
  char **profile_names, **profile_values;
  uint32_t *profile_offsets, profile_num, profile_num_out, profile;
  FILE *tiff_file;

  if (!get_camf_property_list(x3f, "WhiteBalanceColorCorrections",
			      &profile_names, &profile_values, &profile_num) &&
      !get_camf_property_list(x3f, "DP1_WhiteBalanceColorCorrections",
			      &profile_names, &profile_values, &profile_num) &&
      !get_camf_property_list(x3f, "WhiteBalanceIlluminants",
			      &profile_names, &profile_values, &profile_num))
    return X3F_ARGUMENT_ERROR;
  profile_offsets = alloca(profile_num*sizeof(uint32_t));

  TIFFFlush(tiff);
  TIFFSetWriteOffset(tiff, 0);	/* Tell libtiff to do the next write at EOF */
  
  tiff_file = fdopen(dup(TIFFFileno(tiff)), "w+");
  if (!tiff_file) return X3F_OUTFILE_ERROR;
	
  for (profile=0,profile_num_out=0; profile < profile_num; profile++) {
    FILE *tmp;
    TIFF *tmp_tiff;
#define BUFSIZE 1024
    char buf[BUFSIZE];
    int offset, count;

    if (!strcmp(profile_names[profile], get_wb(x3f)))
      continue;     /* The default profile should only be in IFD 0 */

    if (!(tmp = tmpfile())) {
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!(tmp_tiff = TIFFFdOpen(dup(fileno(tmp)), "", "wb"))) { /* Big endian */
      fclose(tmp);
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!write_camera_profile_for_wb(x3f, tmp_tiff, profile_names[profile])) {
      fprintf(stderr,
	      "WARNING: failed to generate camera profile: %s, skipping\n",
	      profile_names[profile]);
      TIFFClose(tmp_tiff);
      fclose(tmp);
      continue;
    }
    TIFFWriteDirectory(tmp_tiff);
    TIFFClose(tmp_tiff);
    
    fseek(tiff_file, 0, SEEK_END);
    offset = (ftell(tiff_file)+1) & ~1; /* 2-byte alignment */
    fseek(tiff_file, offset, SEEK_SET);
    profile_offsets[profile_num_out++] = offset;

    fputs("MMCR", tiff_file);	/* DNG camera profile magic in big endian */
    fseek(tmp, 4, SEEK_SET);	/* Skip over the standard TIFF magic */

    while((count = fread(buf, 1, BUFSIZE, tmp)))
      fwrite(buf, 1, count, tiff_file);

    fclose(tmp);
  }
  
  fclose(tiff_file);
  if (profile_num_out > 0)
    TIFFSetField(tiff, TIFFTAG_EXTRACAMERAPROFILES, 
		 profile_num_out, profile_offsets);
  return X3F_OK;
}
#endif

static int get_camf_rect_as_dngrect(x3f_t *x3f, char *name, uint32_t *rect)
{
  uint32_t camf_rect[4];

  if (!get_camf_rect(x3f, name, camf_rect))
    return 0;
  
  /* Translate from Sigma's to Adobe's view on rectangles */
  rect[0] = camf_rect[1];
  rect[1] = camf_rect[0];
  rect[2] = camf_rect[3] + 1;
  rect[3] = camf_rect[2] + 1;

  return 1;
}

#if 0
static int write_spatial_gain(x3f_t *x3f, TIFF *tiff)
{
  spatial_gain_corr_t corr[MAXCORR];
  int corr_num;
  uint32_t active_area[4], keep_area[4];
  double originv, originh, scalev, scaleh;

  uint8_t *opcode_list, *p;
  int opcode_size[MAXCORR];
  int opcode_list_size = sizeof(dng_opcodelist_header_t);
  dng_opcodelist_header_t *header;
  int i, j;

  if (!get_camf_rect_as_dngrect(x3f, "ActiveImageArea", active_area)) return 0;
  if (!get_camf_rect_as_dngrect(x3f, "KeepImageArea", keep_area)) return 0;

  /* Spatial gain in X3F refers to the entire sensor, but GainMain in
     OpcodeList 2 in DNG refers to the active area */
  originv = (double)((int32_t)keep_area[0] - (int32_t)active_area[0])/
    (active_area[2] - active_area[0]);
  originh = (double)((int32_t)keep_area[1] - (int32_t)active_area[1])/
    (active_area[3] - active_area[1]);
  scalev =
    (double)(keep_area[2] - keep_area[0])/(active_area[2] - active_area[0]);
  scaleh =
    (double)(keep_area[3] - keep_area[1])/(active_area[3] - active_area[1]);

  corr_num = get_spatial_gain(x3f, corr);
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

    /* The image is already cropped to ActiveArea when OpcodeList2 is applied */
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
#endif

/* extern */
x3f_return_t x3f_dump_raw_data_as_dng(x3f_t *x3f,
				      char *outfilename,
				      int denoise,
				      char *wb)
{
  TIFF *f_out;
  uint64_t sub_ifds[1] = {0};
#define THUMBSIZE 100
  uint8_t thumbnail[3*THUMBSIZE];

  double sensor_iso, capture_iso;
  double bmt_to_xyz[9], gain[3], gain_mat[9], dng_to_xyz[9], xyz_to_dng[9];
  float color_matrix[9];
  double gain_inv[3];
  float as_shot_neutral[3];
  uint32_t white_level[3];
  uint32_t active_area[4];
  x3f_area_t image;
  int row;

  if (wb == NULL) wb = get_wb(x3f);

  x3f_dngtags_install_extender();

  if (!image_area(x3f, &image) || image.channels != 3)
    return X3F_ARGUMENT_ERROR;
  if (!preprocess_data(x3f, &image, wb)) return X3F_ARGUMENT_ERROR;
  if (denoise && !run_denoising(x3f)) return X3F_ARGUMENT_ERROR;
  
  f_out = TIFFOpen(outfilename, "w");
  if (f_out == NULL) return X3F_OUTFILE_ERROR;

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
  TIFFSetField(f_out, TIFFTAG_DNGBACKWARDVERSION, "\001\001\000\000");
  TIFFSetField(f_out, TIFFTAG_SUBIFD, 1, sub_ifds);
  /* Prevent clipping of dark areas in DNG processing software */
  TIFFSetField(f_out, TIFFTAG_DEFAULTBLACKRENDER, 1);

  if (get_camf_float(x3f, "SensorISO", &sensor_iso) && 
      get_camf_float(x3f, "CaptureISO", &capture_iso)) {
    double baseline_exposure = log2(capture_iso/sensor_iso);
    TIFFSetField(f_out, TIFFTAG_BASELINEEXPOSURE, baseline_exposure);
  }

  if (!get_bmt_to_xyz(x3f, wb, bmt_to_xyz)) {
    fprintf(stderr, "Could not get bmt_to_xyz for white balance: %s\n", wb);
    TIFFClose(f_out);
    return X3F_ARGUMENT_ERROR;
  }
  if (!get_gain(x3f, wb, gain)) {
    fprintf(stderr, "Could not get gain for white balance: %s\n", wb);
    TIFFClose(f_out);
    return 0;
  }
  x3f_3x3_diag(gain, gain_mat);
  x3f_3x3_3x3_mul(bmt_to_xyz, gain_mat, dng_to_xyz);
  x3f_3x3_inverse(dng_to_xyz, xyz_to_dng);
  vec_double_to_float(xyz_to_dng, color_matrix, 9);
  TIFFSetField(f_out, TIFFTAG_COLORMATRIX1, 9, color_matrix);

  x3f_3x1_invert(gain, gain_inv);
  vec_double_to_float(gain_inv, as_shot_neutral, 3);
  TIFFSetField(f_out, TIFFTAG_ASSHOTNEUTRAL, 3, as_shot_neutral);

  memset(thumbnail, 0, 3*THUMBSIZE); /* TODO: Thumbnail is all black */
  for (row=0; row < 100; row++)
    TIFFWriteScanline(f_out, thumbnail, row, 0);

  TIFFWriteDirectory(f_out);

  TIFFSetField(f_out, TIFFTAG_SUBFILETYPE, 0);
  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, image.columns);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, image.rows);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, 32);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, 3);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LINEARRAW);
  /* Prevent further chroma denoising in DNG processing software */
  TIFFSetField(f_out, TIFFTAG_CHROMABLURRADIUS, 0.0);

  white_level[0] = (uint32_t)(gain[0]*INTERMEDIATE_UNIT);
  white_level[1] = (uint32_t)(gain[1]*INTERMEDIATE_UNIT);
  white_level[2] = (uint32_t)(gain[2]*INTERMEDIATE_UNIT);
  TIFFSetField(f_out, TIFFTAG_WHITELEVEL, 3, white_level);

  if (get_camf_rect_as_dngrect(x3f, "ActiveImageArea", active_area))
    TIFFSetField(f_out, TIFFTAG_ACTIVEAREA, active_area);

  for (row=0; row < image.rows; row++)
    TIFFWriteScanline(f_out, image.data + image.row_stride*row, row, 0);

  TIFFWriteDirectory(f_out);
  TIFFClose(f_out);

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
					    char *wb,
					    int log_hist)
{
  x3f_return_t ret;
  x3f_area_t image;
  FILE *f_out = fopen(outfilename, "wb");
  uint32_t *histogram[3];
  int color, i;
  int row;
  uint16_t max = 0;

  if (f_out == NULL) return X3F_OUTFILE_ERROR;

  ret = get_image(x3f, &image, encoding, crop, 0, wb);
  if (ret != X3F_OK) {
    fclose(f_out);
    return ret;
  }
  if (image.channels < 3) {
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

/* extern */ char *x3f_err(x3f_return_t err)
{
  switch (err) {
  case X3F_OK:             return "ok";
  case X3F_ARGUMENT_ERROR: return "argument error";
  case X3F_INFILE_ERROR:   return "infile error";
  case X3F_OUTFILE_ERROR:  return "outfile error";
  case X3F_INTERNAL_ERROR: return "internal error";
  default:                 return "undefined error";
  }
}

/* --------------------------------------------------------------------- */
/* The End                                                               */
/* --------------------------------------------------------------------- */
