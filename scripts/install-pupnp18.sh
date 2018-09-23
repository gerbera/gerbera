#!/bin/sh
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

unamestr=$(uname)
if [ "$unamestr" == 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

wget https://github.com/mrjimenez/pupnp/archive/release-1.8.3.tar.gz -O pupnp-1.8.3.tgz
tar -xzvf pupnp-1.8.3.tgz
cd pupnp-release-1.8.3
./bootstrap && ./configure $extraFlags --enable-ipv6 --enable-reuseaddr && make && make install\
 && ldconfig
