#!/bin/sh
set -ex
wget http://taglib.github.io/releases/taglib-1.11.1.tar.gz
tar -xzvf taglib-1.11.1.tar.gz
mkdir taglib-build
cd taglib-build
if [ "$CXX" == "clang++" ]; then
	cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" ../taglib-1.11.1
else
	cmake ../taglib-1.11.1
fi
make && sudo make install
