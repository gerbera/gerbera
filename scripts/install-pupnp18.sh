#!/bin/sh
set -ex
wget https://github.com/mrjimenez/pupnp/archive/release-1.8.0.tar.gz -O pupnp-1.8.0.tgz
tar -xzvf pupnp-1.8.0.tgz
cd pupnp-release-1.8.0
./bootstrap && ./configure --prefix=/usr --enable-ipv6 && make && sudo make install
