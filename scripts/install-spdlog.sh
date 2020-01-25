#!/bin/sh
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

VERSION="1.5.0"

unamestr=$(uname)
if [ ! -f spdlog-$VERSION.tgz ]; then
	wget https://github.com/gabime/spdlog/archive/v$VERSION.tar.gz -O spdlog-$VERSION.tgz
fi
tar -xzvf spdlog-$VERSION.tgz
cd spdlog-$VERSION
if [ -d build ]; then
	rm -R build
fi
mkdir build
cd build && \
cmake .. && \
make spdlog && \
make install/fast
