#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# debian/releases/Raspbian.sh - this file is part of Gerbera.
#
# Copyright (C) 2024-2025 Gerbera Contributors
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

echo "Loading functions for ${lsb_distro}"

function install-cmake() {
  echo "::group::Installing CMake"
  sudo apt-get install snapd -y
  sudo snap install core
  sudo snap install cmake --classic
  echo "::endgroup::"
}

function upload_to_repo() {
  #echo "::group::Uploading package"
  echo "No Repo for Raspbian yet"
  #echo "::endgroup::"
}

function set-libraries-dist() {
  libmysqlclient="libmariadb-dev-compat"
}
