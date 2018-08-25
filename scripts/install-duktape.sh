#!/bin/sh
if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

makeCMD="make"
unamestr=$(uname)
if [ "$unamestr" == 'FreeBSD' ]; then
   makeCMD="gmake"
fi

wget http://duktape.org/duktape-2.2.0.tar.xz
tar -xJvf duktape-2.2.0.tar.xz
cd duktape-2.2.0

if [ "$unamestr" == 'Darwin' ]; then
  # Patch Makefile to install on macOS
  # macOS does not support -soname, replace with -install_name
  sed -i -e 's/-soname/-install_name/g' Makefile.sharedlibrary
fi

$makeCMD -f Makefile.sharedlibrary && $makeCMD -f Makefile.sharedlibrary install\
 && ldconfig
