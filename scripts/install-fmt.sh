#!/usr/bin/env bash
set -Eeuo pipefail

VERSION="7.1.3"
UNAME=$(uname)

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

unamestr=$(uname)
if [ ! -f fmt-$VERSION.tgz ]; then
	wget https://github.com/fmtlib/fmt/archive/refs/tags/$VERSION.tar.gz -O fmt-$VERSION.tgz
fi
tar -xzvf fmt-$VERSION.tgz
cd fmt-$VERSION
if [ -d fmtbuild ]; then
	rm -R fmtbuild
fi
mkdir fmtbuild
cd fmtbuild
cmake .. -DBUILD_SHARED_LIBS=YES -DFMT_TEST=NO

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi

make install
