#!/usr/bin/env bash
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${DUKTAPE-2.6.0}"
UNAME=$(uname)

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

script_dir=`pwd -P`
src_dir="${script_dir}/duktape-${VERSION}"
tgz_file="${script_dir}/duktape-${VERSION}.tar.xz"

if [ ! -f "${tgz_file}" ]; then
    wget https://github.com/svaarala/duktape/releases/download/v${VERSION}/duktape-${VERSION}.tar.xz -O "${tgz_file}"
fi

if [ -d "${src_dir}" ]; then
    rm -r ${src_dir}
fi
mkdir "${src_dir}"

tar -xvf "${tgz_file}" --strip-components=1 -C "${src_dir}"

cd "${src_dir}"

if [ "${UNAME}" = 'Darwin' ]; then
    # Patch Makefile to install on macOS
    # macOS does not support -soname, replace with -install_name
    sed -i -e 's/-soname/-install_name/g' Makefile.sharedlibrary
fi

makeCMD="make"
if [ "${UNAME}" = 'FreeBSD' ]; then
    makeCMD="gmake"
fi
if command -v nproc >/dev/null 2>&1; then
    makeCMD="${makeCMD} -j$(nproc)"
fi

$makeCMD -f Makefile.sharedlibrary && $makeCMD -f Makefile.sharedlibrary install || exit 1

if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" != 'alpine' ]; then
        ldconfig
    fi
fi
