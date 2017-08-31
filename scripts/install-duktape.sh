#!/bin/sh
set -ex

makeCMD="make"
unamestr=`uname`
if [ "$unamestr" == 'FreeBSD' ]; then
   makeCMD="gmake"
fi

wget http://duktape.org/duktape-2.1.0.tar.xz
tar -xJvf duktape-2.1.0.tar.xz
cd duktape-2.1.0
$makeCMD -f Makefile.sharedlibrary && sudo $makeCMD -f Makefile.sharedlibrary install
