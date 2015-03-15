  SYS = generic
ifdef COMSPEC
  SYS = windows
else
  OSTYPE = $(shell bash -c 'echo $$OSTYPE')
ifeq (linux-gnu, $(OSTYPE))
  SYS = linux
else
ifeq (darwin, $(findstring darwin,$(OSTYPE)))
  SYS = osx
endif
endif
endif
