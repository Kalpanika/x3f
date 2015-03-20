HOST = generic
ifdef COMSPEC
  HOST = windows
else
  OSTYPE = $(shell bash -c 'echo $$OSTYPE')
ifeq (linux-gnu, $(OSTYPE))
  HOST = linux
else
ifeq (darwin, $(findstring darwin,$(OSTYPE)))
  HOST = osx
endif
endif
endif

ifndef SYS
  SYS = $(HOST)
endif

CC =
CXX =
CMAKE_TOOLCHAIN =
ifeq ($(HOST), $(SYS))
  CC = gcc
  CXX = g++
else				# Cross compilation
ifeq ($(HOST), linux)
ifeq ($(SYS), windows)
  CC = x86_64-w64-mingw32-gcc
  CXX = x86_64-w64-mingw32-g++
  CMAKE_TOOLCHAIN = x86_64-w64-mingw32.cmake
endif
endif
endif

ifndef CC
  $(error Compilation of C for target $(SYS) on host $(HOST) is unsupported)
endif

ifndef CXX
  $(error Compilation of C++ for target $(SYS) on host $(HOST) is unsupported)
endif
