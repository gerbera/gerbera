#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-ffmpegthumbnailer.sh - this file is part of Gerbera.
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

VERSION="${FFMPEGTHUMBNAILER-2.2.2}"

script_dir=`pwd -P`
src_dir="${script_dir}/ffmpegthumbnailer-${VERSION}"
tgz_file="${script_dir}/ffmpegthumbnailer-${VERSION}.tgz"

downloadSource https://github.com/dirkvdb/ffmpegthumbnailer/archive/refs/tags/${VERSION}.tar.gz

cmake .. \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_GIO=ON -DENABLE_THUMBNAILER=ON

makeInstall

ldConfig

exit 0
