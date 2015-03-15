# Set the SYS variable
include sys.mk

.PHONY: default all clean clobber install_opencv

default: all

all clean clobber:
	$(MAKE) -C src $@


OPENCV = deps/lib/$(SYS)/opencv

all: | $(OPENCV)

$(OPENCV) install_opencv:
	./install_opencv.sh $(SYS)
