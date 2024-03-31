#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-libexiv2.sh - this file is part of Gerbera.
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

VERSION="${EXIV2-v0.27.7}"

#script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
script_dir=`pwd -P`
src_dir="${script_dir}/libexiv2-${VERSION}"
tgz_file="${script_dir}/libexiv2-${VERSION}.tar.gz"

BUILD_SHARED=YES
BUILD_XMP=YES
BUILD_BROTLI=YES
BUILD_INIH=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
    if [ "$1" = "head" -o "$2" = "head" ]; then
        BUILD_XMP=NO
        BUILD_BROTLI=NO
        BUILD_INIH=NO
    fi
fi

downloadSource https://github.com/Exiv2/exiv2/archive/refs/tags/${VERSION}.tar.gz

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DEXIV2_ENABLE_XMP=${BUILD_XMP} -DEXIV2_ENABLE_BROTLI=${BUILD_BROTLI} -DEXIV2_ENABLE_INIH=${BUILD_INIH}

makeInstall

ldConfig

exit 0

