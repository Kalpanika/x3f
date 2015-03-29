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
LIPO =

ifeq ($(HOST), $(TARGET))
  CC = gcc
  CXX = g++
ifeq ($(HOST), osx)
  LIPO = lipo
endif
else				# Cross compilation
ifeq ($(HOST_SYS), linux)
ifeq ($(TARGET), windows-x86_64)
  CC = x86_64-w64-mingw32-gcc
  CXX = x86_64-w64-mingw32-g++
  CMAKE_TOOLCHAIN = x86_64-w64-mingw32.cmake
else
ifeq ($(TARGET), windows-i686)
  CC = i686-w64-mingw32-gcc
  CXX = i686-w64-mingw32-g++
  CMAKE_TOOLCHAIN = i686-w64-mingw32.cmake
else
ifeq ($(TARGET), osx-x86_64)
  CC = x86_64-apple-darwin9-gcc
  CXX = x86_64-apple-darwin9-g++
  CMAKE_TOOLCHAIN = x86_64-apple-darwin9.cmake
else
ifeq ($(TARGET), osx-i386)
  CC = i386-apple-darwin9-gcc
  CXX = i386-apple-darwin9-g++
  CMAKE_TOOLCHAIN = i386-apple-darwin9.cmake
else
ifeq ($(TARGET), osx-universal)
  LIPO = x86_64-apple-darwin9-lipo
endif
endif
endif
endif
endif
endif
endif

ifeq ($(TARGET), osx-universal)

ifndef LIPO
  $(error Building universal binaries for target $(TARGET) on host $(HOST) is unsupported)
endif

else

ifndef CC
  $(error C compilation for target $(TARGET) on host $(HOST) is unsupported)
endif

ifndef CXX
  $(error C++ compilation for target $(TARGET) on host $(HOST) is unsupported)
endif

endif

TARGET_SYS = $(shell echo $(TARGET) | sed "s/-.*$$//")
TARGET_CPU = $(shell echo $(TARGET) | sed "s/^$(TARGET_SYS)-//")
