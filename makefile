# Set the SYS variable
include sys.mk

.PHONY: default all clean clobber clean_opencv

default: all

all clean clobber:
	$(MAKE) -C src $@


OPENCV = deps/lib/$(SYS)/opencv

all: | $(OPENCV)/.success

$(OPENCV)/.success:
	./install_opencv.sh $(SYS)

clean_opencv:
	-@rm -r $(OPENCV)
