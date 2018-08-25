#!/bin/bash
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex
LASTFM_VERSION="0.4.0"
wget "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/lastfmlib/lastfmlib-${LASTFM_VERSION}.tar.gz"
tar -xzvf "lastfmlib-${LASTFM_VERSION}.tar.gz"
cd "lastfmlib-${LASTFM_VERSION}"
./configure --prefix=/usr/local
make
make install
ldconfig
