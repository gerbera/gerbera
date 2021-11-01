#!/bin/sh
if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex
. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${LASTFM-0.4.0}"
wget "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/lastfmlib/lastfmlib-${VERSION}.tar.gz"
tar -xzvf "lastfmlib-${VERSION}.tar.gz"
cd "lastfmlib-${VERSION}"
./configure --prefix=/usr/local
make -j$(nproc) && \
make install

if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" != 'alpine' ]; then
        ldconfig
    fi
fi
