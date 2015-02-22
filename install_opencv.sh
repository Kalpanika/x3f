git clone https://github.com/erikrk/opencv.git
cd opencv
git checkout denoising-16bit-abs
cd ..
mkdir opencv_build
mkdir opencv_install
cd opencv_build
cmake -D CMAKE_INSTALL_PREFIX=../opencv_install ../opencv
make -j 16 install
cd ../opencv_install
export LD_LIBRARY_PATH=`pwd`/lib