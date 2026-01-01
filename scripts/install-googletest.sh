#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-googletest.sh - this file is part of Gerbera.
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

VERSION="${GOOGLETEST-1.11.0}"

setFiles gtest tgz
source_files+=("https://github.com/google/googletest/archive/refs/tags/v${VERSION}.tar.gz")
source_files+=("https://github.com/google/googletest/archive/refs/tags/release-${VERSION}.tar.gz")

## Download GoogleTest from GitHub
downloadSource

cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} -std=c++17" -DBUILD_GMOCK=1 ..

makeInstall

ldConfig

exit 0
