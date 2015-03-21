# Set the SYS variable
include sys.mk

.PHONY: default all dist clean clobber clean_opencv

default: all

all dist clean clobber:
	$(MAKE) -C src $@


OPENCV = deps/lib/$(TARGET)/opencv

all dist: | $(OPENCV)/.success

$(OPENCV)/.success:
	./install_opencv.sh $(TARGET) $(CMAKE_TOOLCHAIN)

clean_opencv:
	-@rm -rf deps/lib/*/opencv
