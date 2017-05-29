#!/bin/bash
set -ex
wget http://duktape.org/duktape-2.1.0.tar.xz
tar -xJvf duktape-2.1.0.tar.xz
cd duktape-2.1.0
make -f Makefile.sharedlibrary && sudo make -f Makefile.sharedlibrary install