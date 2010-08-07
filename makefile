.PHONY: all clean clobber

default: all

all clean clobber:
	make -C src $@
