#!/bin/sh
if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

VERSION="1.10"

wget http://github.com/zeux/pugixml/releases/download/v1.10/pugixml-$VERSION.tar.gz -O pugixml-$VERSION.tgz
tar -xzvf pugixml-$VERSION.tgz
mkdir pugixml-build
cd pugixml-build && cmake ../pugixml-$VERSION
make && \
make install || exit 1

. /etc/os-release
if [ "$ID" != 'alpine' ]; then
  ldconfig
fi
