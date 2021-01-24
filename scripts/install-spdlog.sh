#!/usr/bin/env bash
set -Eeuo pipefail

VERSION="1.7.0"
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
if [ -d spdlogbuild ]; then
	rm -R spdlogbuild
fi
mkdir spdlogbuild
cd spdlogbuild
cmake .. -DSPDLOG_FMT_EXTERNAL=ON -DSPDLOG_BUILD_SHARED=OFF
make
make install
