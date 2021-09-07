#!/usr/bin/env bash
set -Eeuo pipefail

VERSION="1.8.5"
UNAME=$(uname)

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

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
cd build

cmake .. -DSPDLOG_FMT_EXTERNAL=ON -DSPDLOG_BUILD_SHARED=OFF
if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install
