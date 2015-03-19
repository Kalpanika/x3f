/* x3f_dngtags.c - libtiff tag extender for DNG tags.
 *
 * The current version of libtiff (4.0.3) only supports tags for DNG
 * versions <= 1.1.0.0
 *
 */

#include "x3f_dngtags.h"

#include <stddef.h>
#include <tiffio.h>

static TIFFExtendProc previous_tag_extender = NULL;

static void tag_extender(TIFF *tiff)
{
  static const TIFFFieldInfo tags[] = {
    {TIFFTAG_EXTRACAMERAPROFILES, -1, -1, TIFF_LONG, FIELD_CUSTOM, 1, 1,
     "ExtraCameraProfiles"},
    {TIFFTAG_PROFILENAME, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 1, 0,
     "ProfileName"},
    {TIFFTAG_ASSHOTPROFILENAME, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 1, 0,
     "AsShotProfileName"},
    {TIFFTAG_OPCODELIST1, -1, -1, TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
     "OpcodeList1"},
    {TIFFTAG_OPCODELIST2, -1, -1, TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
     "OpcodeList2"},
    {TIFFTAG_OPCODELIST3, -1, -1, TIFF_UNDEFINED, FIELD_CUSTOM, 1, 1,
     "OpcodeList3"},
    {TIFFTAG_DEFAULTBLACKRENDER, 1, 1, TIFF_LONG, FIELD_CUSTOM, 1, 0,
     "DefaultBlackRender"},
  };

  TIFFMergeFieldInfo(tiff, tags, sizeof(tags)/sizeof(tags[0]));

  if (previous_tag_extender)
    previous_tag_extender(tiff);
}

void x3f_dngtags_install_extender(void)
{
  static int invoked = 0;

  if (invoked) return;
  invoked = 1;

  previous_tag_extender = TIFFSetTagExtender(tag_extender);
}
