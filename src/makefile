# MAKEFILE
#
# Copyright (c) 2010 - Roland Karlsson (roland@proxel.se)
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
else
ifeq (linux, $(TARGET_SYS))
  EXE =
  CFBASE =
  LDBASE = -lz -ldl -lpthread
else
ifeq (osx, $(TARGET_SYS))
  EXE =
  CFBASE = -arch $(TARGET_CPU) -Wno-pragmas
  LDBASE = -arch $(TARGET_CPU) -lz -liconv -mmacosx-version-min=10.5
else
  $(warning WARNING: Unknown target: $(TARGET))
  EXE =
  CFBASE =
  LDBASE =
endif
endif
endif

OCV = ../deps/lib/$(TARGET)/opencv

ifeq ($(TARGET_SYS), windows)
ZLIB = $(OCV)/share/OpenCV/3rdparty/lib/libzlib.a
else
ZLIB =
endif
OCV_AUX = $(OCV)/share/OpenCV/3rdparty/lib/libtbb.a $(ZLIB)

OCV_INC = $(OCV)/include
OCV_HEADERS := $(shell find $(OCV_INC)/opencv2 -name '*.hpp' -or -name '*.h')
OCV_CFLAGS = -I$(OCV_INC)
OCV_LIBS = $(addprefix $(OCV)/lib/libopencv_,photo.a imgproc.a core.a) $(OCV_AUX)

TIFF_INC1 = ../deps/src/opencv/3rdparty/libtiff
TIFF_INC2 = ../deps/src/$(TARGET)/opencv_build/3rdparty/libtiff
TIFF_HEADERS = $(addprefix $(TIFF_INC1)/,tiffio.h tiff.h tiffvers.h) $(TIFF_INC2)/tif_config.h
TIFF_CFLAGS = -I$(TIFF_INC1) -I$(TIFF_INC2)
TIFF_LIBS = $(OCV)/share/OpenCV/3rdparty/lib/liblibtiff.a $(ZLIB)

CFLAGS = $(CFBASE) -g -O3 -Wall $(C)
CXXFLAGS = $(CFLAGS) -fvisibility-inlines-hidden
LDFLAGS = $(LDBASE) $(L)

BINDIR = ../bin/$(TARGET)

# Build dependencies
# -----------------------------------------------------------

.PHONY: all dist clean clobber

all: $(addprefix $(BINDIR)/,x3f_extract$(EXE) x3f_io_test$(EXE) x3f_matrix_test$(EXE))

$(BINDIR)/x3f_extract$(EXE): $(addprefix $(BINDIR)/,x3f_extract.o x3f_io.o x3f_process.o x3f_meta.o x3f_image.o x3f_print.o x3f_dump.o x3f_matrix.o x3f_dngtags.o x3f_denoise_utils.o x3f_denoise_aniso.o x3f_denoise.o) $(OCV_LIBS) $(TIFF_LIBS)
	$(CXX) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR)/x3f_io_test$(EXE): $(addprefix $(BINDIR)/,x3f_io_test.o x3f_io.o x3f_print.o)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BINDIR)/x3f_extract.o: x3f_extract.c x3f_io.h x3f_process.h x3f_print.h x3f_dump.h x3f_denoise.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_io_test.o: x3f_io_test.c x3f_io.h x3f_print.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_io.o: x3f_io.c x3f_io.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_meta.o: x3f_meta.c x3f_meta.h x3f_io.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_image.o: x3f_image.c x3f_image.h x3f_io.h x3f_meta.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_print.o: x3f_print.c x3f_print.h x3f_io.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_dump.o: x3f_dump.c x3f_dump.h x3f_io.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_process.o: x3f_process.c x3f_process.h x3f_io.h x3f_meta.h x3f_image.h x3f_matrix.h x3f_dngtags.h x3f_denoise.h $(TIFF_HEADERS) | $(BINDIR)
	$(CC) $(CFLAGS) $(TIFF_CFLAGS) $< -c -o $@

$(BINDIR)/x3f_matrix.o: x3f_matrix.c x3f_matrix.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_dngtags.o: x3f_dngtags.c x3f_dngtags.h $(TIFF_HEADERS) | $(BINDIR)
	$(CC) $(CFLAGS) $(TIFF_CFLAGS) $< -c -o $@

$(BINDIR)/x3f_denoise_utils.o: x3f_denoise_utils.cpp x3f_denoise_utils.h x3f_io.h $(OCV_HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(OCV_CFLAGS) $< -c -o $@

$(BINDIR)/x3f_denoise_aniso.o: x3f_denoise_aniso.cpp x3f_denoise_aniso.h x3f_denoise_utils.h x3f_io.h $(OCV_HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(OCV_CFLAGS) $< -c -o $@

$(BINDIR)/x3f_denoise.o: x3f_denoise.cpp x3f_denoise.h x3f_denoise_utils.h x3f_denoise_aniso.h x3f_io.h $(OCV_HEADERS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(OCV_CFLAGS) $< -c -o $@

$(BINDIR)/x3f_matrix_test.o: x3f_matrix_test.c x3f_matrix.h | $(BINDIR)
	$(CC) $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_matrix_test$(EXE): $(addprefix $(BINDIR)/,x3f_matrix_test.o x3f_matrix.o)
	$(CC) $^ -o $@ $(LDFLAGS) -lm

$(BINDIR):
	mkdir $(BINDIR)

# Packaging
# -----------------------------------------------------------

NAME = x3f_tools
VER := $(shell git describe --always --dirty)
DIST = $(NAME)-$(VER)-$(TARGET)
DIST_DIR = ../dist/$(DIST)
DIST_TGZ = ../dist/$(DIST).tar.gz
DIST_ZIP = ../dist/$(DIST).zip
DIST_FILES = README doc/readme.txt doc/copyright.txt bin/x3f_extract$(EXE)
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
	install -D -m 755 -s $< $@

$(DIST_DIR)/%: ../%
	install -D -m 644 $< $@

# Clean up dependencies
# -----------------------------------------------------------

clean:
	-@rm ../bin/*/*.o
	-@rm *~ ../doc/*~
	-@rm -r ../dist/$(NAME)-*/

clobber: clean
	-@rm -r ../bin/*-*/
	-@rm ../dist/$(NAME)-*.tar.gz
	-@rm ../dist/$(NAME)-*.zip
