#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-ffmpegthumbnailer.sh - this file is part of Gerbera.
#
# Copyright (C) 2018-2025 Gerbera Contributors
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

VERSION="${FFMPEGTHUMBNAILER-2.2.2}"
COMMIT="${FFMPEGTHUMBNAILER_commit:-}"

script_dir=`pwd -P`
src_file="https://github.com/dirkvdb/ffmpegthumbnailer/archive/refs/tags/${VERSION}.tar.gz"

if [ -n "${COMMIT}" ]; then
    source_files+=("https://github.com/dirkvdb/ffmpegthumbnailer/archive/${COMMIT}.tar.gz")
    VERSION+="-"
    VERSION+=`echo $COMMIT | cut -c 1-6`
fi
src_dir="${script_dir}/ffmpegthumbnailer-${VERSION}"
tgz_file="${script_dir}/ffmpegthumbnailer-${VERSION}.tgz"
source_files+=("${src_file}")

downloadSource

installDeps ${main_dir} ffmpegthumbnailer

cmake .. \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_GIO=ON -DENABLE_THUMBNAILER=OFF \
  -DENABLE_TESTS=OFF -DENABLE_STATIC=ON -DENABLE_SHARED=ON

makeInstall

ldConfig

exit 0
