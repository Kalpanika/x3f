.PHONY: all clean clobber

default: all

all clean clobber:
	$(MAKE) -C src $@
