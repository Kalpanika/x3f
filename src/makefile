# MAKEFILE
#
# Copyright (c) 2010-2015
# Roland Karlsson (roland@proxel.se)
# BSD-style - see doc/copyright.txt

# -----------------------------------------------------------
# Usage:
# Write "make" shall build the executables on Mac, Linux or Windows.
# Write "make clean" or "make clobber" cleans up less or more.
# -----------------------------------------------------------

# Set the SYS variable
# -----------------------------------------------------------

include ../sys.mk

# Set compilation and load flags etc
# -----------------------------------------------------------

ifeq (windows, $(TARGET_SYS))
  EXE = .exe
  CFBASE =
  LDBASE = -static
  AUXOBJS = mingw_dowildcard.o
else
ifeq (linux, $(TARGET_SYS))
  EXE =
  CFBASE =
  LDBASE = -lz -ldl -lpthread
  AUXOBJS =
else
ifeq (osx, $(TARGET_SYS))
  EXE =
  CFBASE = $(SDKFLAGS) -Wno-pragmas
  LDBASE = $(SDKFLAGS) -Wl,-dead_strip_dylibs -liconv -framework OpenCL
  AUXOBJS =
else
  $(warning WARNING: Unknown target: $(TARGET))
  EXE =
  CFBASE =
  LDBASE =
  AUXOBJS =
endif
endif
endif

OCV = ../deps/lib/$(TARGET)/opencv

ZLIB =
ifneq ($(TARGET_SYS), linux)
ZLIB = $(OCV)/share/OpenCV/3rdparty/lib/libzlib.a
endif
OCV_AUX = $(OCV)/share/OpenCV/3rdparty/lib/libtbb.a $(ZLIB)

OCV_CFLAGS = -I$(OCV)/include
OCV_LIBS = $(addprefix $(OCV)/lib/libopencv_,photo.a imgproc.a core.a) $(OCV_AUX)

TIFF_INC1 = ../deps/src/opencv/3rdparty/libtiff
TIFF_INC2 = ../deps/src/$(TARGET)/opencv_build/3rdparty/libtiff
TIFF_CFLAGS = -I$(TIFF_INC1) -I$(TIFF_INC2)
TIFF_LIBS = $(OCV)/share/OpenCV/3rdparty/lib/liblibtiff.a $(ZLIB)

CFLAGS = $(CFBASE) $(TIFF_CFLAGS) -g -O3 -Wall $(C)
CXXFLAGS = $(CFLAGS) $(OCV_CFLAGS) -fvisibility-inlines-hidden
LDFLAGS = $(LDBASE) $(L)

BINDIR = ../bin/$(TARGET)
PROGS = x3f_extract$(EXE) x3f_io_test$(EXE) x3f_matrix_test$(EXE)

# Build dependencies
# -----------------------------------------------------------

.PHONY: all dist clean clobber

all: $(addprefix $(BINDIR)/,$(PROGS))

ifeq ($(TARGET), osx-universal)

$(BINDIR)/%: ../bin/osx-x86_64/% ../bin/osx-i386/% | $(BINDIR)
	$(LIPO) -create -output $@ $^

.PHONY: force

.SECONDARY: $(addprefix ../bin/osx-x86_64/,$(PROGS))
../bin/osx-x86_64/%: force
	$(MAKE) TARGET=osx-x86_64 $@

.SECONDARY: $(addprefix ../bin/osx-i386/,$(PROGS))
../bin/osx-i386/%: force
	$(MAKE) TARGET=osx-i386 $@

else

-include $(BINDIR)/*.d

$(BINDIR)/x3f_extract$(EXE): $(addprefix $(BINDIR)/,x3f_extract.o x3f_io.o x3f_process.o x3f_meta.o x3f_image.o x3f_spatial_gain.o x3f_output_dng.o x3f_output_tiff.o x3f_output_ppm.o x3f_histogram.o x3f_print_meta.o x3f_dump.o x3f_matrix.o x3f_dngtags.o x3f_denoise_utils.o x3f_denoise_aniso.o x3f_denoise.o x3f_printf.o $(AUXOBJS)) $(OCV_LIBS) $(TIFF_LIBS)
	$(CXX) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR)/x3f_io_test$(EXE): $(addprefix $(BINDIR)/,x3f_io_test.o x3f_io.o x3f_print_meta.o x3f_printf.o $(AUXOBJS))
	$(CC) $^ -o $@ $(LDFLAGS)

$(BINDIR)/x3f_matrix_test$(EXE): $(addprefix $(BINDIR)/,x3f_matrix_test.o x3f_matrix.o x3f_printf.o $(AUXOBJS))
	$(CC) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -MD -o $@

$(BINDIR)/%.o: %.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -c -MD -o $@

endif

$(BINDIR):
	mkdir $(BINDIR)

# Packaging
# -----------------------------------------------------------

NAME = x3f_tools
VER := $(shell git describe --always --dirty --tags)
DIST = $(NAME)-$(VER)-$(TARGET)
DIST_DIR = ../dist/$(DIST)
DIST_TGZ = ../dist/$(DIST).tar.gz
DIST_ZIP = ../dist/$(DIST).zip
DIST_FILES = doc/readme.txt doc/copyright.txt bin/x3f_extract$(EXE)
TARFLAGS = --owner=root --group=root

ifeq ($(TARGET_SYS), windows)
dist: $(DIST_ZIP)
else
dist: $(DIST_TGZ)
endif

$(DIST_TGZ): $(addprefix $(DIST_DIR)/,$(DIST_FILES))
	tar $(TARFLAGS) -czf $@ -C ../dist $(addprefix $(DIST)/,$(DIST_FILES))

$(DIST_ZIP): $(addprefix $(DIST_DIR)/,$(DIST_FILES))
	rm -f $@
	cd ../dist && zip $(abspath $@) $(addprefix $(DIST)/,$(DIST_FILES))

$(DIST_DIR)/bin/%: $(BINDIR)/%
	install $(INSTFLAGS) -D -m 755 -s $< $@

$(DIST_DIR)/%: ../%
	install -D -m 644 $< $@

# Clean up dependencies
# -----------------------------------------------------------

clean:
	-@rm ../bin/*/*.o
	-@rm ../bin/*/*.d
	-@rm *~ ../doc/*~
	-@rm -r ../dist/$(NAME)-*/

clobber: clean
	-@rm -r ../bin/*-*/
	-@rm ../dist/$(NAME)-*.tar.gz
	-@rm ../dist/$(NAME)-*.zip
