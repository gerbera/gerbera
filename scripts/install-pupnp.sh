#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-pupnp.sh - this file is part of Gerbera.
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

VERSION="${PUPNP-1.14.12}"

setFiles pupnp tgz
source_files+=("https://github.com/pupnp/pupnp/archive/release-${VERSION}.tar.gz")

downloadSource

installDeps ${main_dir} pupnp

(
    cd "${src_dir}"
    bash ./bootstrap
)

if [ "${UNAME}" = 'FreeBSD' ]; then
    extraFlags=""
else
    extraFlags="--prefix=/usr/local"
fi

../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --disable-blocking-tcp-connections

makeInstall

ldConfig

exit 0
