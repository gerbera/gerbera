#!/usr/bin/env bash
set -Eeuo pipefail

PUPNP_VERSION="1.14.10"
UNAME=$(uname)

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

script_dir=`pwd -P`
src_dir="${script_dir}/pupnp-${PUPNP_VERSION}"
tgz_file="${script_dir}/pupnp-${PUPNP_VERSION}.tgz"

set -ex

wget https://github.com/pupnp/pupnp/archive/release-${PUPNP_VERSION}.tar.gz -O "${tgz_file}"

if [ -d "${src_dir}" ]; then
  rm -r ${src_dir}
fi
mkdir "${src_dir}"
tar -xzvf "${tgz_file}" --strip-components=1 -C "${src_dir}"
cd "${src_dir}"

./bootstrap
if [ "${UNAME}" = 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --disable-blocking-tcp-connections
if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install

. /etc/os-release
if [ "$ID" != 'alpine' ]; then
  ldconfig
fi
