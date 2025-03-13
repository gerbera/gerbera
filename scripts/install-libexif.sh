#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-libexif.sh - this file is part of Gerbera.
#
# Copyright (C) 2024-2025 Gerbera Contributors
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

VERSION="${EXIF-v0.6.24}"
COMMIT="${EXIF_commit:-}"

script_dir=`pwd -P`
src_file="https://github.com/libexif/libexif/archive/refs/tags/${VERSION}.tar.gz"

if [[ -n "$COMMIT" ]]; then
    source_files+=("https://github.com/libexif/libexif/archive/${COMMIT}.tar.gz")
    VERSION+="-"
    VERSION+=`echo $COMMIT | cut -c 1-6`
fi

src_dir="${script_dir}/libexif-${VERSION}"
tgz_file="${script_dir}/libexif-${VERSION}.tar.gz"
source_files+=("${src_file}")

downloadSource

installDeps ${main_dir} libexif

(
  cd ${src_dir}
  autoreconf -i
)
../configure --srcdir=..

makeInstall

ldConfig

exit 0
