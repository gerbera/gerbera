#!/usr/bin/env bash
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${GOOGLETEST-1.11.0}"

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

script_dir=`pwd -P`
src_dir="${script_dir}/gtest-${VERSION}"
tgz_file="${script_dir}/gtest-${VERSION}.tgz"

##
## Install the latest version of GoogleTest
##

## Download GoogleTest from GitHub
wget https://github.com/google/googletest/archive/refs/tags/release-${VERSION}.tar.gz  -O "${tgz_file}"

if [ -d "${src_dir}" ]; then
  rm -r ${src_dir}
fi
mkdir "${src_dir}"
tar -xvf "${tgz_file}" --strip-components=1 -C "${src_dir}"
cd "${src_dir}"

## Build GoogleTest using CMake

if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -std=c++17" ..

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install

if [ "$(uname)" != 'Darwin' ]; then
    ldconfig
fi
