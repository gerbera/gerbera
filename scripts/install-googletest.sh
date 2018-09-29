#!/usr/bin/env bash
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
##
## Install the latest version of GoogleTest
##
pwd=$(pwd)
unamestr=$(uname)
if [ -f "$pwd/master.zip" ]; then rm -f "$pwd/master.zip"; fi
if [ -d "$pwd/googletest-master" ]; then rm -Rf "$pwd/googletest-master"; fi

## Download GoogleTest from GitHub
wget https://github.com/google/googletest/archive/master.zip
unzip master.zip
cd googletest-master || exit 1

## Build GoogleTest using CMake
mkdir build && cd build || exit 1
if [[ $CXX == clang++* ]]; then
	cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++ -std=c++14" ../
else
	cmake -DCMAKE_CXX_FLAGS="-std=c++14" ../
fi
make && make install

if [ "$unamestr" != 'Darwin' ]; then
    ldconfig
fi

cd "$pwd" || exit 1
