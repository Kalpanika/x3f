# Set the SYS variable
include sys.mk

.PHONY: all clean clobber install_opencv

default: all

all clean clobber:
	$(MAKE) -C src $@

install_opencv:
	./install_opencv.sh $(SYS)
