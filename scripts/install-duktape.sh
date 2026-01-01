#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-duktape.sh - this file is part of Gerbera.
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

VERSION="${DUKTAPE-2.6.0}"
COMMIT="${DUKTAPE_commit:-}"

src_file="https://github.com/svaarala/duktape/releases/download/v${VERSION}/duktape-${VERSION}.tar.xz"

if [ -n "${COMMIT}" ]; then
    source_files+=("https://github.com/svaarala/duktape/archive/${COMMIT}.tar.xz")
    VERSION+="-"
    VERSION+=`echo $COMMIT | cut -c 1-6`
fi

setFiles duktape tar.xz
source_files+=("${src_file}")

downloadSource

cd "${src_dir}"

MAKEFILE="Makefile.sharedlibrary"

if [ "${UNAME}" = 'Darwin' ]; then
    # Patch Makefile to install on macOS
    # macOS does not support -soname, replace with -install_name
    sed -i -e 's/-soname/-install_name/g' ${MAKEFILE}
fi

makeCMD="make"
if [ "${UNAME}" = 'FreeBSD' ]; then
    makeCMD="gmake"
fi
if command -v nproc >/dev/null 2>&1; then
    makeCMD="${makeCMD} -j$(nproc)"
fi

$makeCMD -f ${MAKEFILE} && $makeCMD -f ${MAKEFILE} install || exit 1

ldConfig

exit 0
