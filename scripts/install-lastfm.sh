#!/bin/bash
set -ex
LASTFM_VERSION="0.4.0"
wget "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/lastfmlib/lastfmlib-${LASTFM_VERSION}.tar.gz"
tar -xzvf "lastfmlib-${LASTFM_VERSION}.tar.gz"
cd "lastfmlib-${LASTFM_VERSION}"
./configure --prefix=/usr
make
make install
