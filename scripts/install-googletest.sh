#!/usr/bin/env bash
##
## Install the latest version of GoogleTest
##
pwd=$(pwd)
if [ -f "$pwd/master.zip" ]; then rm -f $pwd/master.zip; fi
if [ -d "$pwd/googletest-master" ]; then rm -Rf $pwd/googletest-master; fi

## Download GoogleTest from GitHub
wget https://github.com/google/googletest/archive/master.zip
unzip master.zip
cd googletest-master

## Build GoogleTest using CMake
mkdir build && cd build
if [[ $CXX == clang++* ]]; then
	cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" ../
else
	cmake ../
fi
make && sudo make install && sudo ldconfig

cd $pwd
