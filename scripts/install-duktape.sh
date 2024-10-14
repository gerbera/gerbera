#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-duktape.sh - this file is part of Gerbera.
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

VERSION="${DUKTAPE-2.6.0}"

script_dir=`pwd -P`
src_dir="${script_dir}/duktape-${VERSION}"
tgz_file="${script_dir}/duktape-${VERSION}.tar.xz"

downloadSource https://github.com/svaarala/duktape/releases/download/v${VERSION}/duktape-${VERSION}.tar.xz

cd "${src_dir}"

if [ "${UNAME}" = 'Darwin' ]; then
    # Patch Makefile to install on macOS
    # macOS does not support -soname, replace with -install_name
    sed -i -e 's/-soname/-install_name/g' Makefile.sharedlibrary
fi

makeCMD="make"
if [ "${UNAME}" = 'FreeBSD' ]; then
    makeCMD="gmake"
fi
if command -v nproc >/dev/null 2>&1; then
    makeCMD="${makeCMD} -j$(nproc)"
fi

$makeCMD -f Makefile.sharedlibrary && $makeCMD -f Makefile.sharedlibrary install || exit 1

ldConfig

exit 0
