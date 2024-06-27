#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# install-pugixml.sh - this file is part of Gerbera.
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

VERSION="${PUGIXML-1.11.4}"

#script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)
script_dir=`pwd -P`
src_dir="${script_dir}/pugixml-${VERSION}"
tgz_file="${script_dir}/pugixml-${VERSION}.tgz"

downloadSource https://github.com/zeux/pugixml/releases/download/v${VERSION}/pugixml-${VERSION}.tar.gz

cmake .. -DPUGIXML_BUILD_TESTS=OFF

makeInstall

ldConfig

exit 0
