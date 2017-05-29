#!/bin/bash
set -ex
wget https://github.com/mrjimenez/pupnp/archive/release-1.8.1.tar.gz -O pupnp-1.8.1.tgz
tar -xzvf pupnp-1.8.1.tgz
cd pupnp-release-1.8.1
./bootstrap && ./configure --prefix=/usr --enable-ipv6 --enable-reuseaddr && make && sudo make install
