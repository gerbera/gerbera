#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# debian/releases/bullseye.sh - this file is part of Gerbera.
#
# Copyright (C) 2024-2026 Gerbera Contributors
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

echo "Loading functions for ${lsb_codename}"

function install-cmake() {
  echo "::group::Installing CMake"
  sudo apt-get install build-essential -y
  sudo bash "${GRB_SH_DIR}install-cmake.sh"
  echo "::endgroup::"
}

function install-cmake-backport() {
  echo "::group::Installing CMake"
  sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
  sudo apt-add-repository "deb http://deb.debian.org/debian ${lsb_codename}-backports main"
  sudo apt-get update -y
  sudo apt-get install cmake-data=3.25.\* cmake=3.25.\* -yf
  echo "::endgroup::"
}

function set-libraries-rel() {
  libduktape="duktape-dev libduktape205"
}
