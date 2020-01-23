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
if [ -f "$pwd/gtest.tgz" ]; then rm -f "$pwd/gtest.tgz"; fi
if [ -d "$pwd/gtest" ]; then rm -Rf "$pwd/gtest"; fi

## Download GoogleTest from GitHub
wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz -O gtest.tgz
mkdir gtest || exit
tar -xf gtest.tgz --strip 1 -C gtest|| exit 1
cd gtest || exit 1

## Build GoogleTest using CMake
mkdir build && cd build || exit 1
cmake -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS -std=c++17" ../ || exit 1

make -j$(nproc) && make install

if [ "$unamestr" != 'Darwin' ]; then
    ldconfig
fi

cd "$pwd" || exit 1
