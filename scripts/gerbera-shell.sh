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


set -Eeuo pipefail

if [[ -z "${ROOT_DIR-}" ]]; then
  GRB_SH_DIR=$(dirname "$0")
  GRB_SH_DIR=$(realpath "${GRB_SH_DIR}")/
else
  GRB_SH_DIR="${ROOT_DIR}scripts/"
fi

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
