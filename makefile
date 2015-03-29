# Set the SYS variable
include sys.mk

.PHONY: default all dist dist-all dist-32 dist-64 clean clobber clean_opencv

default: all

all dist clean clobber:
	$(MAKE) -C src $@

ifeq ($(HOST), linux-x86_64)
dist-all: dist-64 dist-32

dist-32:
	$(MAKE) TARGET=windows-i686 dist

dist-64:
	$(MAKE) TARGET=linux-x86_64 dist
	$(MAKE) TARGET=windows-x86_64 dist
endif

ifeq ($(TARGET), osx-universal)

all dist: deps/lib/osx-x86_64/opencv/.success deps/lib/osx-i386/opencv/.success

deps/lib/osx-x86_64/opencv/.success:
	$(MAKE) TARGET=osx-x86_64 $@

deps/lib/osx-i386/opencv/.success:
	$(MAKE) TARGET=osx-i386 $@

else

OPENCV = deps/lib/$(TARGET)/opencv

all dist: | $(OPENCV)/.success

$(OPENCV)/.success:
	./install_opencv.sh $(TARGET) $(CMAKE_TOOLCHAIN)

endif

clean_opencv:
	-@rm -rf deps/lib/*/opencv
