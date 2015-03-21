HOST_SYS =
HOST_CPU =

ifdef COMSPEC
  HOST_SYS = windows
  HOST_CPU = $(shell uname -m)
else
  UNAME = $(shell uname -s)
ifeq ($(UNAME), Linux)
  HOST_SYS = linux
  HOST_CPU = $(shell uname -m)
else
ifeq ($(UNAME), Darwin)
  HOST_SYS = osx
  HOST_CPU = universal
endif
endif
endif

ifndef HOST_SYS
  HOST_SYS = generic
  $(warning WARNING: Could not determine host system, assuming $(HOST_SYS))
endif

ifndef HOST_CPU
  HOST_CPU = generic
  $(warning WARNING: Could not determine host CPU, assuming $(HOST_CPU))
endif

HOST = $(HOST_SYS)-$(HOST_CPU)

ifndef TARGET
  TARGET = $(HOST)
endif

CC =
CXX =
CMAKE_TOOLCHAIN =

ifeq ($(HOST), $(TARGET))
  CC = gcc
  CXX = g++
else				# Cross compilation
ifeq ($(HOST_SYS), linux)
ifeq ($(TARGET), windows-x86_64)
  CC = x86_64-w64-mingw32-gcc
  CXX = x86_64-w64-mingw32-g++
  CMAKE_TOOLCHAIN = x86_64-w64-mingw32.cmake
endif
endif
endif

ifndef CC
  $(error C compilation for target $(TARGET) on host $(HOST) is unsupported)
endif

ifndef CXX
  $(error C++ compilation for target $(TARGET) on host $(HOST) is unsupported)
endif

TARGET_SYS = $(shell echo $(TARGET) | sed "s/-.*$$//")
TARGET_CPU = $(shell echo $(TARGET) | sed "s/^$(TARGET_SYS)-//")
