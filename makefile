# Set the SYS variable
include sys.mk

.PHONY: default all dist dist-all clean clobber clean_opencv

default: all

all dist clean clobber:
	$(MAKE) -C src $@

ifeq ($(HOST), linux-x86_64)
dist-all:
	$(MAKE) TARGET=linux-x86_64 dist
	$(MAKE) TARGET=windows-x86_64 dist
	$(MAKE) TARGET=windows-i686 dist
endif

OPENCV = deps/lib/$(TARGET)/opencv

all dist: | $(OPENCV)/.success

$(OPENCV)/.success:
	./install_opencv.sh $(TARGET) $(CMAKE_TOOLCHAIN)

clean_opencv:
	-@rm -rf deps/lib/*/opencv
