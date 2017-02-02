#!/bin/sh
set -ex
wget http://taglib.github.io/releases/taglib-1.11.1.tar.gz
tar -xzvf taglib-1.11.1.tar.gz
mkdir taglib-build
cd taglib-build
cmake ../taglib-1.11.1
make && sudo make install
