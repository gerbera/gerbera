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

VERSION="${PUPNP-1.14.30}"
COMMIT="${PUPNP_COMMIT:-}"

if [[ -n "$COMMIT" ]]; then
    source_files+=("https://github.com/pupnp/pupnp/archive/${COMMIT}.tar.gz")
    VERSION+="-"
    VERSION+=`echo $COMMIT | cut -c 1-6`
fi

setFiles pupnp tgz
source_files+=("https://github.com/pupnp/pupnp/archive/release-${VERSION}.tar.gz")

downloadSource

installDeps ${main_dir} pupnp

if [ "${UNAME}" = 'FreeBSD' ]; then
    extraFlags=""
else
    extraFlags="--prefix=/usr/local"
fi

MODE="2"
if [[ -f ${src_dir}/bootstrap ]]; then
    MODE="1"
fi
if [[ ${MODE} = "1" ]]; then

    (
        cd "${src_dir}"
        bash ./bootstrap
    )
    ../configure --srcdir=.. $extraFlags --enable-ipv6 --enable-reuseaddr --disable-blocking-tcp-connections
else
    cmake .. -DUPNP_BUILD_SAMPLES=OFF \
             -DBUILD_SHARED_LIBS=ON \
             -DUPNP_BUILD_STATIC=ON \
             -DUPNP_ENABLE_BLOCKING_TCP_CONNECTIONS=OFF \
             -DUPNP_ENABLE_HELPER_API_TOOLS=ON \
             -DUPNP_MINISERVER_REUSEADDR=ON \
             -DUPNP_ENABLE_IPV6=ON \
             -DUPNP_ENABLE_TESTING=OFF
fi

makeInstall

ldConfig

exit 0
