#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-taglib.sh - this file is part of Gerbera.
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

main_dir=$(dirname "${BASH_SOURCE[0]}")
main_dir=$(realpath "${main_dir}")/
. ${main_dir}/versions.sh

VERSION="${TAGLIB-1.12}"

script_dir=`pwd -P`
src_dir="${main_dir}/taglib-${VERSION}"
tgz_file="${main_dir}/taglib-${VERSION}.tgz"
patch_file="${main_dir}/${lsb_distro}/taglib.patch"

downloadSource https://github.com/taglib/taglib/archive/v${VERSION}.tar.gz "-" "${patch_file}"

installDeps ${main_dir} taglib

cmake .. -DBUILD_EXAMPLES=OFF

makeInstall

ldConfig

exit 0
