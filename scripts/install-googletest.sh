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
if [ -f "$pwd/release-1.8.1.zip" ]; then rm -f "$pwd/release-1.8.1.zip"; fi
if [ -d "$pwd/googletest-release-1.8.1" ]; then rm -Rf "$pwd/googletest-release-1.8.1"; fi

## Download GoogleTest from GitHub
wget https://github.com/google/googletest/archive/release-1.8.1.zip
unzip release-1.8.1.zip
cd googletest-release-1.8.1 || exit 1

## Build GoogleTest using CMake
mkdir build && cd build || exit 1
if [[ $CXX == clang++* ]]; then
	cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" ../
else
	cmake ../
fi
make && make install

if [ "$unamestr" != 'Darwin' ]; then
    ldconfig
fi

cd "$pwd" || exit 1
