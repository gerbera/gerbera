#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-spdlog.sh - this file is part of Gerbera.
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

VERSION="${SPDLOG-1.9.2}"

BUILD_SHARED=YES

if [ $# -gt 0 ]; then
    if [ "$1" = "static" ]; then
        BUILD_SHARED=NO
    fi
fi

script_dir=`pwd -P`
src_dir="${script_dir}/spdlog-${VERSION}"
tgz_file="${script_dir}/spdlog-${VERSION}.tgz"

downloadSource https://github.com/gabime/spdlog/archive/v$VERSION.tar.gz

cmake .. -DSPDLOG_FMT_EXTERNAL=ON -DBUILD_SHARED_LIBS=${BUILD_SHARED}

makeInstall

ldConfig

exit 0
