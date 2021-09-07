#!/usr/bin/env bash
set -Eeuo pipefail

VERSION="7.1.3"

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

if [ ! -f fmt-$VERSION.tgz ]; then
    wget https://github.com/fmtlib/fmt/archive/refs/tags/$VERSION.tar.gz -O fmt-$VERSION.tgz
fi
tar -xzvf fmt-$VERSION.tgz
cd fmt-$VERSION
if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DFMT_TEST=NO

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install
