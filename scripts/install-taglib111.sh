#!/bin/bash
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex
wget http://taglib.github.io/releases/taglib-1.11.1.tar.gz
tar -xzvf taglib-1.11.1.tar.gz
mkdir taglib-build
cd taglib-build
cmake ../taglib-1.11.1
make -j$(nproc) && make install && ldconfig
