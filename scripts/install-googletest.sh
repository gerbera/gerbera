#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-googletest.sh - this file is part of Gerbera.
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

VERSION="${GOOGLETEST-1.11.0}"

script_dir=`pwd -P`
src_dir="${script_dir}/gtest-${VERSION}"
tgz_file="${script_dir}/gtest-${VERSION}.tgz"

## Download GoogleTest from GitHub
downloadSource https://github.com/google/googletest/archive/refs/tags/release-${VERSION}.tar.gz https://github.com/google/googletest/archive/refs/tags/v${VERSION}.tar.gz

cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -std=c++17" -DBUILD_GMOCK=1 ..

makeInstall

ldConfig

exit 0
