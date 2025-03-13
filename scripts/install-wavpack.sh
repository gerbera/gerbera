#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-wavpack.sh - this file is part of Gerbera.
#
# Copyright (C) 2021-2025 Gerbera Contributors
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

VERSION="${WAVPACK-5.4.0}"

script_dir=`pwd -P`
src_dir="${script_dir}/wavpack-${VERSION}"
tgz_file="${script_dir}/wavpack-${VERSION}.tar.xz"
source_files+=("https://github.com/dbry/WavPack/releases/download/${VERSION}/wavpack-${VERSION}.tar.xz")

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

downloadSource

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED}

if [[ -f ../autogen.sh ]]; then
    ../autogen.sh
fi

../configure \
    --disable-asm \
    --enable-man \
    --enable-rpath \
    --disable-tests \
    --disable-apps \
    --enable-dsd \
    --enable-legacy

makeInstall

ldConfig

exit 0
