#!/bin/bash

if [ -z $1 ]; then
    echo usage $0 "<TARGET> [<cmake toolchain file>]"
    echo Please run '"make"'
    exit 1
else
    TARGET=$1
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
LIB=$ROOT/deps/lib/$TARGET

OCV_SRC=$SRC/opencv
OCV_BLD=$SRC/$TARGET/opencv_build
OCV_LIB=$LIB/opencv

OCV_URL=https://github.com/erikrk/opencv.git
OCV_HASH=01d3df0d00e5c802108b90bd2dedb50e9a9ecacf

OCV_FLAGS="-D CMAKE_BUILD_TYPE=RELEASE -D BUILD_SHARED_LIBS=OFF \
           -D WITH_IPP=OFF -D WITH_TBB=ON -D BUILD_TBB=ON \
           -D BUILD_TIFF=ON -D WITH_JPEG=OFF -D WITH_JASPER=OFF \
           -D WITH_PNG=OFF -D WITH_WEBP=OFF -D WITH_OPENEXR=OFF \
           -D BUILD_TESTS=OFF -D BUILD_PERF_TESTS=OFF -D BUILD_DOCS=OFF \
           -D BUILD_opencv_apps=OFF"

if [[ $TARGET =~ ^windows- ]]; then
    OCV_FLAGS="$OCV_FLAGS -D BUILD_ZLIB=ON \
                          -D OPENCV_EXTRA_CXX_FLAGS=-DUSE_WINTHREAD"
fi

if [ -n $2 ]; then
    OCV_FLAGS="$OCV_FLAGS -D CMAKE_TOOLCHAIN_FILE=$ROOT/$2"
fi

if [ -e $OCV_SRC ]; then
    echo Fetch opencv
    cd $OCV_SRC || exit 1
    git fetch || exit 1
else
    echo Clone opencv
    mkdir -p $SRC || exit 1
    git clone $OCV_URL $OCV_SRC || exit 1
    cd $OCV_SRC || exit 1
fi

echo Build Opencv
git checkout $OCV_HASH || exit 1
mkdir -p $OCV_BLD || exit 1
mkdir -p $OCV_LIB || exit 1
cd $OCV_BLD || exit 1
cmake $OCV_FLAGS -D CMAKE_INSTALL_PREFIX=$OCV_LIB $OCV_SRC || exit 1
make -j $CORES install || exit 1
touch $OCV_LIB/.success || exit 1
echo Successfully installed OpenCV in $OCV_LIB
