#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-cxxopts.sh - this file is part of Gerbera.
#
# Copyright (C) 2026 Gerbera Contributors
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

VERSION="${CXXOPTS-3.2.1}"
COMMIT="${CXXOPTS_COMMIT:-}"

src_file="https://github.com/jarro2783/cxxopts/archive/refs/tags/v${VERSION}.tar.gz"

if [[ -n "$COMMIT" ]]; then
    source_files+=("https://github.com/jarro2783/cxxopts/archive/${COMMIT}.tar.gz")
    VERSION+="-"
    VERSION+=`echo $COMMIT | cut -c 1-6`
fi

setFiles cxxopts tar.gz
source_files+=("${src_file}")
downloadSource

installDeps ${main_dir} cxxopts

cmake .. -DCXXOPTS_BUILD_TESTS=OFF \
         -DCXXOPTS_BUILD_EXAMPLES=OFF \
         -DCXXOPTS_ENABLE_WARNINGS=OFF

makeInstall

exit 0
