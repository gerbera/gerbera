#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# build-solaris.sh - this file is part of Gerbera.
#
# Copyright (C) 2025 Gerbera Contributors
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

# Pre reqs
pkg install sudo bash curl git gnu-coreutils

PATH=/usr/gnu/bin:$PATH

SCRIPT_DIR=$(dirname "$0")
SCRIPT_DIR=$(realpath "${SCRIPT_DIR}")/
ROOT_DIR=$(dirname "$0")/../../
ROOT_DIR=$(realpath "${ROOT_DIR}")/

if [[ -f "${ROOT_DIR}scripts/gerbera-shell.sh" ]]; then
  . "${ROOT_DIR}scripts/gerbera-shell.sh"
else
  echo "No script library found at ${ROOT_DIR}scripts/gerbera-shell.sh"
fi

function install-gcc() {
  echo "::group::Installing GCC"
  set +e
  pkg install gcc14
  set -e
  echo "::endgroup::"
}

function install-cmake() {
  echo "::group::Installing CMake"
  set +e
  pkg install cmake
  set -e
  echo "::endgroup::"
}

function set-libraries-dist() {
  echo "No ${lsb_distro} Libraries"
}

function set-libraries-rel() {
  echo "No ${lsb_codename} Libraries"
}

function set-libraries() {
  echo ""
}

function install-deb-s3() {
  echo ""
}

function upload_to_repo() {
  echo "::group::Uploading package"
  echo "not yet implemented"
  echo "::endgroup::"
}

function install-ffmpegthumbnailer() {
  echo "::group::Installing ffmpegthumbnailer"
  "${GRB_SH_DIR}install-ffmpegthumbnailer.sh"
  echo "::endgroup::"
}

echo "::group::Preparing Libraries"

# Work around https://github.com/actions/checkout/issues/760
GH_ACTIONS="${GH_ACTIONS:-n}"
if [[ "${GH_ACTIONS}" == "y" ]]; then
  git config --global --add safe.directory "$(pwd)"
fi

if [[ -f ${SCRIPT_DIR}releases/${lsb_distro}-${lsb_codename}.sh ]]; then
  . ${SCRIPT_DIR}releases/${lsb_distro}-${lsb_codename}.sh
fi
if [[ -f ${SCRIPT_DIR}releases/${lsb_codename}.sh ]]; then
  . ${SCRIPT_DIR}releases/${lsb_codename}.sh
fi
if [[ -f ${SCRIPT_DIR}releases/${lsb_distro}.sh ]]; then
  . ${SCRIPT_DIR}releases/${lsb_distro}.sh
fi

my_sys=${lsb_codename}
my_upnp=pupnp
if [ $# -gt 0 ]; then
  my_sys=$1
fi
if [ $# -gt 1 ]; then
  my_upnp=$2
fi

echo "Running $0 ${my_sys} ${my_upnp} for ${lsb_distro}-${lsb_arch}"

set-libraries
set-libraries-dist
set-libraries-rel

echo "::endgroup::"

BUILD_DIR="build-solaris"

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir "${BUILD_DIR}"

  install-gcc
  install-cmake
  set +euEo pipefail

  echo "::group::Installing dependencies"
  pkg install \
            gnu-make icu4c ffmpeg ninja \
            pkg-config sqlite-3 libexif taglib \
            autoconf automake gnu-m4 libtool gnu-tar gzip bzip2

  export PKG_CONFIG_PATH=/usr/local/lib:/usr/local/lib/pkgconfig:/opt/ooce/lib/pkgconfig:/opt/ooce/lib/amd64/pkgconfig:${PKG_CONFIG_PATH}
  set -euEo pipefail
  echo "::endgroup::"

  mkdir libs
  (
    cd libs
    install-wavpack
    install-duktape
    install-jsoncpp
    install-pugixml
    install-fmt
    install-spdlog
    install-ffmpegthumbnailer
    install-matroska
    # install-libexiv2 head
    install-pupnp
    # install-taglib

    install-googletest
  )
fi

commit_date=$(git log -1 --date=format:"%Y%m%d%H%M%S" --format="%ad")
set +euEo pipefail
export LD_LIBRARY_PATH=/usr/local/lib:/opt/ooce/lib:/opt/ooce/lib/amd64:${LD_LIBRARY_PATH}
echo "-> LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
git_ver=$(git describe --tags | sed 's/\(.*\)-.*/\1/' | sed s/-/+/ | sed s/v//)
if [ -z "${git_ver}" ]; then
  git_ver="_git"
fi
set -euEo pipefail

cd "${BUILD_DIR}"
# If version contains a + this is a non-tag build so add commit date
sol_version="${git_ver}-${lsb_codename}"
is_tag=1
if [[ ${git_ver} == *"+"* ]]; then
  sol_version="${git_ver}~${commit_date}-${lsb_codename}"
  is_tag=0
fi
if [[ ${GH_EVENT:-} == "pull_request" ]]; then
  is_tag=2
fi

sol_name="gerbera_${sol_version}_${lsb_arch}.deb"

echo "::group::SystemD build support"
set +e
SYSTEMD_PATH=$(pkg-config --variable=systemdsystemunitdir systemd)
WITH_SYSTEMD="ON"
if [[ -z "${SYSTEMD_PATH}" ]]; then
  sudo pkg install \
      systemd-dev
  SYSTEMD_PATH=$(pkg-config --variable=systemdsystemunitdir systemd)
fi
if [[ -z "${SYSTEMD_PATH}" ]]; then
  WITH_SYSTEMD="OFF"
  echo "SystemD build support not available"
else
  echo "SystemD build support enabled: ${SYSTEMD_PATH}"
fi
set -e
echo "::endgroup::"

set -euEo pipefail
cmake_preset=sunos-develop

echo "::group::Building gerbera"
cmake "${ROOT_DIR}" --preset="${cmake_preset}" \
  -DWITH_SYSTEMD=${WITH_SYSTEMD}
make "-j$(nproc)"
echo "::endgroup::"

echo "::group::Testing gerbera"
ctest --output-on-failure
echo "::endgroup::"
# cpack -G TGZ \
#      -D CPACK_PACKAGE_VERSION="${sol_version}"
