#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# gerbera-shell.sh - this file is part of Gerbera.
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

lsb_codename=""
lsb_distro=""
lsb_rel=""
lsb_arch="$(uname -p)"
if [ -x "$(command -v dpkg)" ]; then
    lsb_arch=$(dpkg --print-architecture)
fi

if [ -x "$(command -v lsb_release)" ]; then
    lsb_codename=$(lsb_release -c --short)
    if [[ "${lsb_codename}" == "n/a" ]]; then
        lsb_codename="unstable"
    fi
    lsb_distro=$(lsb_release -i --short)
    lsb_rel=$(lsb_release -r --short)
    lsb_codename=${lsb_codename,,}
    lsb_distro=${lsb_distro,,}
    lsb_rel=${lsb_rel,,}
fi
if [ -z "${lsb_codename}" ]; then
    lsb_codename="$(uname -o)"
    lsb_distro="$(uname)"
    lsb_rel="unknown"
    lsb_codename=${lsb_codename,,}
    lsb_distro=${lsb_distro,,}
    lsb_rel=${lsb_rel,,}
fi

if [[ -z "${ROOT_DIR-}" ]]; then
  GRB_SH_DIR=$(dirname "$0")
  GRB_SH_DIR=$(realpath "${GRB_SH_DIR}")/
else
  GRB_SH_DIR="${ROOT_DIR}scripts/"
fi

set -Eeuo pipefail

function install-fmt() {
  echo "::group::Installing fmt"
  sudo bash "${GRB_SH_DIR}install-fmt.sh" static
  echo "::endgroup::"
}

function install-spdlog() {
  echo "::group::Installing spdlog"
  sudo bash "${GRB_SH_DIR}install-spdlog.sh" static
  echo "::endgroup::"
}

function install-libpqxx() {
  echo "::group::Installing libpqxx"
  sudo bash "${GRB_SH_DIR}install-libpqxx.sh" $*
  echo "::endgroup::"
}

function install-libzippp() {
  echo "::group::Installing libzippp"
  sudo bash "${GRB_SH_DIR}install-libzip.sh" $*
  sudo bash "${GRB_SH_DIR}install-libzippp.sh" $*
  echo "::endgroup::"
}

function install-pupnp() {
  echo "::group::Installing libupnp"
  sudo bash "${GRB_SH_DIR}install-pupnp.sh"
  echo "::endgroup::"
}

function install-npupnp() {
  echo "::group::Installing libnpupnp"
  sudo bash "${GRB_SH_DIR}install-npupnp.sh"
  echo "::endgroup::"
}

function install-googletest() {
  echo "::group::Installing googletest"
  sudo bash "${GRB_SH_DIR}install-googletest.sh"
  echo "::endgroup::"
}

function install-taglib() {
  echo "::group::Installing taglib"
  sudo bash "${GRB_SH_DIR}install-taglib.sh"
  echo "::endgroup::"
}

function install-pugixml() {
  echo "::group::Installing pugixml"
  sudo bash "${GRB_SH_DIR}install-pugixml.sh"
  echo "::endgroup::"
}

function install-duktape() {
  echo "::group::Installing duktape"
  sudo bash "${GRB_SH_DIR}install-duktape.sh"
  echo "::endgroup::"
}

function install-libexif() {
  echo "::group::Installing libexif"
  sudo bash "${GRB_SH_DIR}install-libexif.sh"
  echo "::endgroup::"
}

function install-libexiv2() {
  echo "::group::Installing libexiv2"
  sudo bash "${GRB_SH_DIR}install-libexiv2.sh" static $*
  echo "::endgroup::"
}

function install-matroska() {
  echo "::group::Installing matroska"
  sudo bash "${GRB_SH_DIR}install-ebml.sh"
  sudo bash "${GRB_SH_DIR}install-matroska.sh"
  echo "::endgroup::"
}

function install-ffmpegthumbnailer() {
  echo "::group::Installing ffmpegthumbnailer"
  sudo bash "${GRB_SH_DIR}install-ffmpegthumbnailer.sh"
  echo "::endgroup::"
}

function install-wavpack() {
  echo "::group::Installing WavPACK"
  sudo bash "${GRB_SH_DIR}install-wavpack.sh"
  echo "::endgroup::"
}

function install-jsoncpp() {
  echo "::group::Installing JsonCPP"
  sudo bash "${GRB_SH_DIR}install-jsoncpp.sh"
  echo "::endgroup::"
}
