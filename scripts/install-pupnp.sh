#!/bin/sh
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

VERSION="1.12.0"

unamestr=$(uname)
if [ "$unamestr" == 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

wget https://github.com/mrjimenez/pupnp/archive/release-$VERSION.tar.gz -O pupnp-$VERSION.tgz

# Backport C++ fix
wget https://github.com/pupnp/pupnp/commit/5a8e93f1a57cce5cead5c8c566a75f7c7c294c97.patch -O cxx.patch

tar -xzvf pupnp-$VERSION.tgz
cd pupnp-release-$VERSION
# Backport C++ fix
patch -p1 < ../cxx.patch
./bootstrap && \
./configure $extraFlags --enable-ipv6 --enable-reuseaddr && \
make -j$(nproc) && \
make install

. /etc/os-release
if [ "$ID" != 'alpine' ]; then
  ldconfig
fi
