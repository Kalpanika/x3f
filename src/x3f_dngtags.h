/* x3f_dngtags.h - libtiff tag extender for DNG tags.
 *
 * The current vestion of lbtiff (4.0.3) only supports tag for DNG
 * version <= 1.1.0.0
 *
 * Copyright (c) 2014 - Roland Karlsson (roland@proxel.se)
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_DNGTAGS_H
#define X3F_DNGTAGS_H

#define PHOTOMETRIC_CFA 32803
#define PHOTOMETRIC_LINEARRAW 34892

#define TIFFTAG_EXTRACAMERAPROFILES 50933
#define TIFFTAG_PROFILENAME 50936
#define TIFFTAG_ASSHOTPROFILENAME 50934

extern void x3f_dngtags_install_extender(void);

#endif
