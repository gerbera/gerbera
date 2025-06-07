#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-ebml.sh - this file is part of Gerbera.
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

VERSION="${EBML-1.4.2}"

setFiles libebml tar.gz
source_files+=("https://github.com/Matroska-Org/libebml/archive/refs/tags/release-${VERSION}.tar.gz")

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

downloadSource

cmake .. -DBUILD_SHARED_LIBS=${BUILD_SHARED}

makeInstall

ldConfig

exit 0
