#!/usr/bin/env bash
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${EXIV2-v0.27.7}"

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

#script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
script_dir=`pwd -P`
src_dir="${script_dir}/libexiv2-${VERSION}"
tgz_file="${script_dir}/libexiv2-${VERSION}.tar.gz"

BUILD_SHARED=YES
BUILD_XMP=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
    if [ "$1" = "head" -o "$2" = "head" ]; then
        BUILD_XMP=NO
    fi
fi

if [ ! -f "${tgz_file}" ]; then
    wget https://github.com/Exiv2/exiv2/archive/refs/tags/${VERSION}.tar.gz -O "${tgz_file}"
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

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DEXIV2_ENABLE_XMP=${BUILD_XMP}

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
