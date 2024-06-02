#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# inclide-focal.sh - this file is part of Gerbera.
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

echo "Loading functions for ${lsb_codename}"

function install-cmake() {
  echo "::group::Installing CMake"
  sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
  curl https://apt.kitware.com/keys/kitware-archive-latest.asc \
	  | gpg --dearmor - \
	  | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
  sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${lsb_codename} main"
  sudo apt-get update -y
  sudo apt-get install cmake -y
  echo "::endgroup::"
}

function set-libraries-rel() {
  libduktape="duktape-dev libduktape205"
}

function install-deb-s3() {
  sudo apt-get install -y ruby gpg
  if [[ "${deb_arch:-}" == "armhf"  ]]; then
    gem sources -r https://rubygems.org/
    echo -e "---\n :ssl_verify_mode: 0\n :backtrace: false\n :bulk_threshold: 1000\n :sources: ["http://rubygems.org"]\n :update_sources: true\n :verbose: true\n" > ~/.gemrc
  fi
  sudo gem install deb-s3
}
