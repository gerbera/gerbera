#!/usr/bin/env bash
set -Eeuo pipefail

PUPNP_VERSION="1.14.0"
UNAME=$(uname)

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)

cleanup() {
  rm "${script_dir}/pupnp.tgz"
  rm -rf "${script_dir}/pupnp-src"
}
trap cleanup SIGINT SIGTERM ERR EXIT

wget https://github.com/pupnp/pupnp/archive/release-${PUPNP_VERSION}.tar.gz -O "${script_dir}/pupnp.tgz"

mkdir "${script_dir}/pupnp-src"
tar -xf "${script_dir}/pupnp.tgz" --strip-components=1 -C "${script_dir}/pupnp-src"
cd "${script_dir}/pupnp-src"

./bootstrap
if [ "${UNAME}" = 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

./configure $extraFlags --enable-ipv6 --enable-reuseaddr
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
