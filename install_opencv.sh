#!/bin/sh

source ./lib_path

cd ..

echo
echo Check out opencv
git clone https://github.com/erikrk/opencv.git
cd opencv
git checkout denoising-16bit-abs
cd ..

echo
echo Build and install opencv
mkdir opencv_build
mkdir opencv_install
cd opencv_build
cmake -D CMAKE_INSTALL_PREFIX=../opencv_install ../opencv
make -j 16 install

echo
echo Ready, now you can run make
