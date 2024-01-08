#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-googletest.sh - this file is part of Gerbera.
#
# Copyright (C) 2018-2024 Gerbera Contributors
#
# Gerbera is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# Gerbera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# NU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
# $Id$
set -Eeuo pipefail

. $(dirname "${BASH_SOURCE[0]}")/versions.sh

VERSION="${GOOGLETEST-1.11.0}"

if ! [ "$(id -u)" = 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi
set -ex

script_dir=`pwd -P`
src_dir="${script_dir}/gtest-${VERSION}"
tgz_file="${script_dir}/gtest-${VERSION}.tgz"

##
## Install the latest version of GoogleTest
##

## Download GoogleTest from GitHub
set +e
wget https://github.com/google/googletest/archive/refs/tags/release-${VERSION}.tar.gz -O "${tgz_file}"
if [ $? -eq 8 ]; then
    set -e
    wget https://github.com/google/googletest/archive/refs/tags/v${VERSION}.tar.gz -O "${tgz_file}"
else
    exit
fi
set -e

if [ -d "${src_dir}" ]; then
  rm -r ${src_dir}
fi
mkdir "${src_dir}"
tar -xvf "${tgz_file}" --strip-components=1 -C "${src_dir}"
cd "${src_dir}"

## Build GoogleTest using CMake

if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -std=c++17" ..

if command -v nproc >/dev/null 2>&1; then
    make "-j$(nproc)"
else
    make
fi
make install

if [ "$(uname)" != 'Darwin' ]; then
    if [ -f /etc/os-release ]; then
        . /etc/os-release
    else
        ID="Linux"
    fi
    if [ "${ID}" == "alpine" ]; then
        ldconfig /
    else
        ldconfig
    fi
fi

exit 0
