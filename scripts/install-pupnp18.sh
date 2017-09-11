#!/bin/sh
set -ex

unamestr=`uname`
if [ "$unamestr" == 'FreeBSD' ]; then
   extraFlags=""
elif [ "$unamestr" == 'Darwin' ]; then
   extraFlags="--prefix=/usr/local"
else
   extraFlags="--prefix=/usr"
fi

wget https://github.com/mrjimenez/pupnp/archive/release-1.8.1.tar.gz -O pupnp-1.8.1.tgz
tar -xzvf pupnp-1.8.1.tgz
cd pupnp-release-1.8.1
./bootstrap && ./configure $extraFlags --enable-ipv6 --enable-reuseaddr && make && sudo make install
