#!/bin/sh
# Gerbera - https://gerbera.io/
#
# install-lastfm.sh - this file is part of Gerbera.
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
set -ex

main_dir=$(dirname "${BASH_SOURCE[0]}")
main_dir=$(realpath "${main_dir}")/
. ${main_dir}/versions.sh

VERSION="${LASTFM-0.4.0}"
wget "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/lastfmlib/lastfmlib-${VERSION}.tar.gz"
tar -xzvf "lastfmlib-${VERSION}.tar.gz"
cd "lastfmlib-${VERSION}"
./configure --prefix=/usr/local

makeInstall

ldConfig

exit 0
