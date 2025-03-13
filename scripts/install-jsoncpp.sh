#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-jsoncpp.sh - this file is part of Gerbera.
#
# Copyright (C) 2025 Gerbera Contributors
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

VERSION="${JSONCPP-1.9.6}"

script_dir=`pwd -P`
src_dir="${script_dir}/jsoncpp-${VERSION}"
tgz_file="${script_dir}/jsoncpp-${VERSION}.tar.gz"
source_files+=("https://github.com//open-source-parsers/jsoncpp/archive/refs/tags/${VERSION}.tar.gz")

BUILD_STATIC=ON
BUILD_SHARED=ON

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=OFF
    fi
fi

downloadSource

installDeps ${main_dir} jsoncpp

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} \
         -DBUILD_STATIC_LIBS=${BUILD_STATIC} \
         -DJSONCPP_WITH_EXAMPLE=OFF \
         -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
         -DJSONCPP_WITH_TESTS=OFF

makeInstall

ldConfig

exit 0
