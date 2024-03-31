#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-pupnp.sh - this file is part of Gerbera.
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

VERSION="${PUPNP-1.14.12}"

script_dir=`pwd -P`
src_dir="${script_dir}/pupnp-${VERSION}"
tgz_file="${script_dir}/pupnp-${VERSION}.tgz"

downloadSource https://github.com/pupnp/pupnp/archive/release-${VERSION}.tar.gz

(
    cd "${src_dir}"
    ./bootstrap
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
