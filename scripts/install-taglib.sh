#!/bin/sh
if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex
#wget http://taglib.github.io/releases/taglib-1.12.0.tar.gz
wget https://github.com/taglib/taglib/archive/v1.12.tar.gz -O taglib-1.12.0.tar.gz
tar -xzvf taglib-1.12.0.tar.gz

rm -R taglib-build
mkdir taglib-build

cd taglib-build
cmake ../taglib-1.12
make -j$(nproc) && make install && ldconfig
