#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# do-release.sh - this file is part of Gerbera.
#
# Copyright (C) 2021-2026 Gerbera Contributors
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

set -xe

sed -i "s#GERBERA_VERSION [[:digit:]].[[:digit:]].[[:digit:]]_git#GERBERA_VERSION $1#" CMakeLists.txt
sed -i "s#version = u'[[:digit:]].[[:digit:]].[[:digit:]]'#version = u'$1'#" doc/conf.py
sed -i "s#release = u'[[:digit:]].[[:digit:]].[[:digit:]]_git'#release = u'$1'#" doc/conf.py

git add CMakeLists.txt doc/conf.py
git commit -m "Gerbera $1"
git tag v$1

sed -i "s#GERBERA_VERSION $1#GERBERA_VERSION $2_git#" CMakeLists.txt
sed -i "s#version = u'$1'#version = u'$2'#" doc/conf.py
sed -i "s#release = u'$1'#release = u'$2_git'#" doc/conf.py

git add CMakeLists.txt doc/conf.py
git commit -m "Bump master"
