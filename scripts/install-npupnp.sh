#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-npupnp.sh - this file is part of Gerbera.
#
# Copyright (C) 2024 Gerbera Contributors
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

VERSION="${NPUPNP-6.1.0}"

script_dir=`pwd -P`
src_dir="${script_dir}/npupnp-${VERSION}"
tgz_file="${script_dir}/npupnp-${VERSION}.tgz"

downloadSource https://framagit.org/medoc92/npupnp/-/archive/libnpupnp-v${VERSION}/npupnp-libnpupnp-v${VERSION}.tar.gz

(
    cd "${src_dir}"
    ./autogen.sh
)

if [ "${UNAME}" = 'FreeBSD' ]; then
    extraFlags=""
else
    extraFlags="--prefix=/usr/local"
fi

../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --enable-tools --enable-static

makeInstall

ldConfig

exit 0
