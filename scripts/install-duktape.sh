#!/usr/bin/env bash
set -Eeuo pipefail

DUK_VER="2.6.0"
UNAME=$(uname)

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)

cleanup() {
  rm "${script_dir}/duktape.tar.xz"
  rm -rf "${script_dir}/duk-src"
}
trap cleanup SIGINT SIGTERM ERR EXIT

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

wget http://duktape.org/duktape-$DUK_VER.tar.xz -O "${script_dir}/duktape.tar.xz"
mkdir "${script_dir}/duk-src"
tar -xf "${script_dir}/duktape.tar.xz" --strip-components=1 -C "${script_dir}/duk-src"

cd "${script_dir}/duk-src"

if [ "${UNAME}" = 'Darwin' ]; then
  # Patch Makefile to install on macOS
  # macOS does not support -soname, replace with -install_name
  sed -i -e 's/-soname/-install_name/g' Makefile.sharedlibrary
fi

makeCMD="make"
if [ "${UNAME}" = 'FreeBSD' ]; then
   makeCMD="gmake"
fi

$makeCMD -f Makefile.sharedlibrary && $makeCMD -f Makefile.sharedlibrary install || exit 1

. /etc/os-release
if [ "$ID" != 'alpine' ]; then
  ldconfig
fi
