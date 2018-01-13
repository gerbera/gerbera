#!/bin/sh
set -ex

unamestr=`uname`
if [ "$unamestr" == 'FreeBSD' ]; then
   extraFlags=""
else
   extraFlags="--prefix=/usr/local"
fi

wget https://github.com/mrjimenez/pupnp/archive/release-1.8.3.tar.gz -O pupnp-1.8.3.tgz
tar -xzvf pupnp-1.8.3.tgz
cd pupnp-release-1.8.3
./bootstrap && ./configure $extraFlags --enable-ipv6 --enable-reuseaddr && make && sudo make install\
 && sudo ldconfig
