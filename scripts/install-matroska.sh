#!/usr/bin/env bash
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${MATROSKA-1.6.3}"

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

#script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
script_dir=`pwd -P`
src_dir="${script_dir}/libmatroska-${VERSION}"
tgz_file="${script_dir}/libmatroska-${VERSION}.tar.gz"

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

if [ ! -f "${tgz_file}" ]; then
    wget https://github.com/Matroska-Org/libmatroska/archive/refs/tags/release-${VERSION}.tar.gz -O "${tgz_file}"
fi

if [ -d "${src_dir}" ]; then
    rm -r ${src_dir}
fi
mkdir "${src_dir}"

tar -xzvf "${tgz_file}" --strip-components=1 -C "${src_dir}"

cd "${src_dir}"

if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED}

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install

if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" != 'alpine' ]; then
        ldconfig
    fi
fi
