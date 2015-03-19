#!/bin/sh

if [ -z $1 ]; then
    echo usage $0 "<SYS>"
    echo Please run '"make"'
    exit 1
else
    SYS=$1
fi

CORES=`nproc 2>/dev/null`
if [ $? -eq 0 ]; then
    echo Detected $CORES cores
else
    CORES=4
    echo Unable to detect number of cores, assuming $CORES
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
mkdir -p $OCV_BLD || exit 1
mkdir -p $OCV_LIB || exit 1
cd $OCV_BLD || exit 1
cmake -D CMAKE_INSTALL_PREFIX=$OCV_LIB -D CMAKE_BUILD_TYPE=RELEASE \
      -D WITH_TBB=ON -D BUILD_TBB=ON -D BUILD_TIFF=ON -D BUILD_SHARED_LIBS=OFF \
       $OCV_SRC || exit 1
make -j $CORES install || exit 1
cd $ROOT || exit 1

touch $OCV_LIB/.success || exit 1
echo Successfully installed OpenCV in $OCV_LIB
