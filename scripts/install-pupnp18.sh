#!/bin/sh
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

VERSION="1.8.4"

unamestr=$(uname)
if [ "$unamestr" == 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

wget https://github.com/mrjimenez/pupnp/archive/release-$VERSION.tar.gz -O pupnp-$VERSION.tgz
tar -xzvf pupnp-$VERSION.tgz
cd pupnp-release-$VERSION
./bootstrap && \
./configure $extraFlags --enable-ipv6 --enable-reuseaddr && \
make && \
make install && \
ldconfig
