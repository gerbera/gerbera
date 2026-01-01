#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-matroska.sh - this file is part of Gerbera.
#
# Copyright (C) 2018-2026 Gerbera Contributors
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

VERSION="${MATROSKA-1.6.3}"

setFiles libmatroska tar.gz
source_files+=("https://github.com/Matroska-Org/libmatroska/archive/refs/tags/release-${VERSION}.tar.gz")

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "${1}" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

downloadSource

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DCMAKE_POLICY_VERSION_MINIMUM=3.5

makeInstall

ldConfig

exit 0
