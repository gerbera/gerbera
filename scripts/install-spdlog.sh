#!/usr/bin/env bash
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${SPDLOG-1.9.2}"

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

script_dir=`pwd -P`
src_dir="${script_dir}/spdlog-${VERSION}"
tgz_file="${script_dir}/spdlog-${VERSION}.tgz"

if [ ! -f "${tgz_file}" ]; then
    wget https://github.com/gabime/spdlog/archive/v$VERSION.tar.gz -O ${tgz_file}
fi

if [ -d "${src_dir}" ]; then
    rm -r ${src_dir}
fi
mkdir "${src_dir}"

tar -xzvf ${tgz_file} --strip-components=1 -C ${src_dir}
cd ${src_dir}
if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

cmake .. -DSPDLOG_FMT_EXTERNAL=ON -DBUILD_SHARED_LIBS=${BUILD_SHARED}

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install
