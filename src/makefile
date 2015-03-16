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

ifeq (windows, $(SYS))
  EXE = .exe
  CFBASE =
  LDBASE =
else
ifeq (linux, $(SYS))
  EXE =
  CFBASE =
  LDBASE =
else
ifeq (osx, $(SYS))
  EXE =
  ARCH = -arch i386 -arch x86_64
  CFBASE = $(ARCH)
  LDBASE = $(ARCH)
else
  EXE =
  CFBASE =
  LDBASE =
endif
endif
endif

OPENCV = ../deps/lib/$(SYS)/opencv
OPENCV_INC = $(OPENCV)/include
OPENCV_HEADERS = $(shell find $(OPENCV_INC) -name '*.hpp' -or -name '*.h')
OPENCV_LIBS = -lopencv_core -lopencv_photo -lopencv_imgproc

CFLAGS = $(CFBASE) -g -O3 -Wall $(C)
LDFLAGS = $(LDBASE) $(L)

BINDIR = ../bin/$(SYS)
VPATH = $(OPENCV)/lib		# Used by make to find prerequisites

# Build dependencies
# -----------------------------------------------------------

all: $(addprefix $(BINDIR)/,x3f_extract$(EXE) x3f_io_test$(EXE) x3f_matrix_test$(EXE))

$(BINDIR)/x3f_extract$(EXE): $(addprefix $(BINDIR)/,x3f_extract.o x3f_io.o x3f_process.o x3f_meta.o x3f_image.o x3f_print.o x3f_dump.o x3f_matrix.o x3f_dngtags.o x3f_denoise_utils.o x3f_denoise_aniso.o x3f_denoise.o) $(OPENCV_LIBS)
	g++ $^ -o $@ $(LDFLAGS) -lm -ltiff

$(BINDIR)/x3f_io_test$(EXE): $(addprefix $(BINDIR)/,x3f_io_test.o x3f_io.o x3f_print.o)
	gcc $^ -o $@ $(LDFLAGS)

$(BINDIR)/x3f_extract.o: x3f_extract.c x3f_io.h x3f_process.h x3f_print.h x3f_dump.h x3f_denoise.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_io_test.o: x3f_io_test.c x3f_io.h x3f_print.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_io.o: x3f_io.c x3f_io.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_meta.o: x3f_meta.c x3f_meta.h x3f_io.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_image.o: x3f_image.c x3f_image.h x3f_io.h x3f_meta.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_print.o: x3f_print.c x3f_print.h x3f_io.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_dump.o: x3f_dump.c x3f_dump.h x3f_io.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_process.o: x3f_process.c x3f_process.h x3f_io.h x3f_meta.h x3f_image.h x3f_matrix.h x3f_dngtags.h x3f_denoise.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_matrix.o: x3f_matrix.c x3f_matrix.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_dngtags.o: x3f_dngtags.c x3f_dngtags.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_denoise_utils.o: x3f_denoise_utils.cpp x3f_denoise_utils.h x3f_io.h $(OPENCV_HEADERS) | $(BINDIR)
	g++ $(CFLAGS) -I$(OPENCV_INC) $< -c -o $@

$(BINDIR)/x3f_denoise_aniso.o: x3f_denoise_aniso.cpp x3f_denoise_aniso.h x3f_denoise_utils.h x3f_io.h $(OPENCV_HEADERS) | $(BINDIR)
	g++ $(CFLAGS) -I$(OPENCV_INC) $< -c -o $@

$(BINDIR)/x3f_denoise.o: x3f_denoise.cpp x3f_denoise.h x3f_denoise_utils.h x3f_denoise_aniso.h x3f_io.h $(OPENCV_HEADERS) | $(BINDIR)
	g++ $(CFLAGS) -I$(OPENCV_INC) $< -c -o $@

$(BINDIR)/x3f_matrix_test.o: x3f_matrix_test.c x3f_matrix.h | $(BINDIR)
	gcc $(CFLAGS) $< -c -o $@

$(BINDIR)/x3f_matrix_test$(EXE): $(addprefix $(BINDIR)/,x3f_matrix_test.o x3f_matrix.o)
	gcc $^ -o $@ $(LDFLAGS) -lm

$(BINDIR):
	mkdir $(BINDIR)

# Clean up dependencies
# -----------------------------------------------------------

clean:
	-@rm ../bin/*/*.o
	-@rm ../bin/*/.dummy
	-@rm *~ ../doc/*~

clobber: clean
	-@rm -r ../bin/windows ../bin/osx ../bin/linux ../bin/generic
