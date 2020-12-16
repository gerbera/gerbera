#!/usr/bin/env bash
set -Eeuo pipefail

cleanup() {
  rm -rf "${script_dir}/build"
  rm "${script_dir}/gtest.tgz"
  rm -rf "${script_dir}/gtest"
}
trap cleanup SIGINT SIGTERM ERR EXIT

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
##
## Install the latest version of GoogleTest
##

## Download GoogleTest from GitHub
mkdir "${script_dir}/gtest"
wget https://github.com/google/googletest/archive/release-1.10.0.tar.gz -O "${script_dir}/gtest.tgz"
tar -xf "${script_dir}/gtest.tgz" --strip 1 -C "${script_dir}/gtest"

## Build GoogleTest using CMake
mkdir "${script_dir}/build"
cd "${script_dir}/build"
cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -std=c++17" ../gtest

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install

if [ "$(uname)" != 'Darwin' ]; then
    ldconfig
fi
