#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-npupnp.sh - this file is part of Gerbera.
#
# Copyright (C) 2024 Gerbera Contributors
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

VERSION="${NPUPNP-6.1.0}"
UNAME=$(uname)

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

script_dir=`pwd -P`
src_dir="${script_dir}/npupnp-${VERSION}"
tgz_file="${script_dir}/npupnp-${VERSION}.tgz"

if [ ! -f "${tgz_file}" ]; then
    wget https://framagit.org/medoc92/npupnp/-/archive/libnpupnp-v${VERSION}/npupnp-libnpupnp-v${VERSION}.tar.gz -O "${tgz_file}"
fi

if [ -d "${src_dir}" ]; then
    rm -r ${src_dir}
fi
mkdir "${src_dir}"
tar -xzvf "${tgz_file}" --strip-components=1 -C "${src_dir}"
cd "${src_dir}"

./autogen.sh
if [ "${UNAME}" = 'FreeBSD' ]; then
    extraFlags=""
else
    extraFlags="--prefix=/usr/local"
fi

if [ -d build ]; then
    rm -R build
fi
mkdir build
cd build

../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --enable-tools --enable-static
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
