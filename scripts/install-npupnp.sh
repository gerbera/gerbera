#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-npupnp.sh - this file is part of Gerbera.
#
# Copyright (C) 2024-2026 Gerbera Contributors
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

main_dir=$(dirname "${BASH_SOURCE[0]}")
main_dir=$(realpath "${main_dir}")/
. ${main_dir}/gerbera-install-shell.sh
. ${main_dir}/versions.sh

VERSION="${NPUPNP-6.1.0}"

setFiles npupnp tgz
source_files+=("https://framagit.org/medoc92/npupnp/-/archive/libnpupnp-v${VERSION}/npupnp-libnpupnp-v${VERSION}.tar.gz")

downloadSource

MODE="6"
if [[ -f ./autogen.sh ]]; then
    MODE="5"
fi
if [[ ${MODE} = "5" ]]; then
(
    cd "${src_dir}"
    ./autogen.sh
)
fi

installDeps ${main_dir} npupnp

if [ "${UNAME}" = 'FreeBSD' ]; then
    extraFlags=""
else
    extraFlags="--prefix=/usr/local"
fi

if [[ ${MODE} = "5" ]]; then
    ../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --enable-tools --enable-static
    makeInstall
else
    (
    cd "${src_dir}"
    meson setup $extraFlags --default-library both -D expat=enabled build
    meson configure --buildtype release build
    )
    ninja -v
    meson install
fi

ldConfig

exit 0
