#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-libzip.sh - this file is part of Gerbera.
#
# Copyright (C) 2025-2026 Gerbera Contributors
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

VERSION="${LIBZIP-v1.11.4}"

setFiles libzip tar.gz
source_files+=("https://github.com/nih-at/libzip/archive/refs/tags/${VERSION}.tar.gz")

BUILD_STATIC=ON
BUILD_SHARED=ON

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=OFF
    fi
fi

downloadSource

installDeps ${main_dir} libzip

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} \
         -DBUILD_TOOLS=OFF \
         -DBUILD_REGRESS=OFF \
         -DBUILD_OSSFUZZ=OFF \
         -DBUILD_EXAMPLES=OFF \
         -DENABLE_BZIP2=${BUILD_SHARED} \
         -DENABLE_ZSTD=${BUILD_SHARED} \
         -DENABLE_LZMA=${BUILD_SHARED} \
         -DENABLE_COMMONCRYPTO=${BUILD_SHARED} \
         -DENABLE_OPENSSL=${BUILD_SHARED} \
         -DENABLE_GNUTLS=${BUILD_SHARED} \
         -DENABLE_MBEDTLS=${BUILD_SHARED} \
         -DBUILD_DOC=OFF

makeInstall

ldConfig

exit 0
