#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-libpqxx.sh - this file is part of Gerbera.
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

VERSION="${PQXX-7.10.1}"

setFiles pqxx tar.gz
source_files+=("https://github.com/jtv/libpqxx/archive/refs/tags/${VERSION}.tar.gz")

BUILD_SHARED=ON

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=OFF
    fi
fi

downloadSource

installDeps ${main_dir} pqxx

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} \
         -DBUILD_DOC=OFF \
         -DSKIP_BUILD_TEST=ON

makeInstall

ldConfig

exit 0
