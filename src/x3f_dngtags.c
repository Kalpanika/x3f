/* x3f_dngtags.c - libtiff tag extender for DNG tags.
 *
 * The current vestion of lbtiff (4.0.3) only supports tag for DNG
 * version <= 1.1.0.0
 *
 * Copyright (c) 2014 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_dngtags.h"

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
  };

  TIFFMergeFieldInfo(tiff, tags, sizeof(tags)/sizeof(tags[0]));

  if (previous_tag_extender)
    previous_tag_extender(tiff);
}

void x3f_dngtags_install_extender(void)
{
  static int invoked = 0;

  if (invoked) return;

  previous_tag_extender = TIFFSetTagExtender(tag_extender); 
}
