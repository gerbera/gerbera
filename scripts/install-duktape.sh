#!/bin/bash
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

DUK_VER="2.5.0"

makeCMD="make"
unamestr=$(uname)
if [ "$unamestr" == 'FreeBSD' ]; then
   makeCMD="gmake"
fi

wget http://duktape.org/duktape-$DUK_VER.tar.xz
tar -xJvf duktape-$DUK_VER.tar.xz
cd duktape-$DUK_VER

if [ "$unamestr" == 'Darwin' ]; then
  # Patch Makefile to install on macOS
  # macOS does not support -soname, replace with -install_name
  sed -i -e 's/-soname/-install_name/g' Makefile.sharedlibrary
fi

$makeCMD -f Makefile.sharedlibrary && $makeCMD -f Makefile.sharedlibrary install\
 && ldconfig
