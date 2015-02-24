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

if [ -e $OCV_LIB ] ; then
    echo Seems like opencv already is installed in $OCV_LIB
    exit 1
fi

if [ -e $OCV_SRC ] ; then
    echo Seems like opencv already is downloaded in $OCV_SRC,
    echo but not installed in $OCV_LIB
    echo this can have several causes
    echo look in the install script and do the rest manually
    exit 1
fi

echo Clone opencv
mkdir -p $SRC
git clone https://github.com/erikrk/opencv.git $OCV_SRC
cd $OCV_SRC
git checkout denoising-16bit-master
cd $ROOT

echo Build Opencv
source ./lib_path
mkdir -p $OCV_BLD
mkdir -p $OCV_LIB
cd $OCV_BLD
cmake -D CMAKE_INSTALL_PREFIX=$OCV_LIB $OCV_SRC
make install
cd $ROOT

echo Ready, now you can make "'make'"
