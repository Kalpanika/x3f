#!/bin/sh

if [ -z $1 ]; then
    echo usage $0 "<SYS>"
    echo Please run '"make install_opencv"'
else
    SYS=$1
fi

ROOT=$PWD
SRC=$ROOT/deps/src
LIB=$ROOT/deps/lib/$SYS

OCV_SRC=$SRC/opencv
OCV_BLD=$SRC/$SYS/opencv_build
OCV_LIB=$LIB/opencv

if [ -e $OCV_SRC ] ; then
    echo Pull opencv
    cd $OCV_SRC || exit 1
    git checkout denoising-16bit-master || exit 1
    git pull || exit 1
    cd $ROOT || exit 1
else
    echo Clone opencv
    mkdir -p $SRC || exit 1
    git clone https://github.com/erikrk/opencv.git $OCV_SRC || exit 1
    cd $OCV_SRC || exit 1
    git checkout denoising-16bit-master || exit 1
    cd $ROOT || exit 1
fi

echo Build Opencv
source ./lib_path
mkdir -p $OCV_BLD || exit 1
mkdir -p $OCV_LIB || exit 1
cd $OCV_BLD || exit 1
cmake -D CMAKE_INSTALL_PREFIX=$OCV_LIB \
      -D CMAKE_BUILD_TYPE=RELEASE -D WITH_TBB=ON $OCV_SRC || exit 1
make -j 4 install || exit 1
cd $ROOT || exit 1

echo Ready, now you can run "'make'"
