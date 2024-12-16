#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# build-suse.sh - this file is part of Gerbera.
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
set -Eeuo pipefail

SCRIPT_DIR=$(dirname "$0")
SCRIPT_DIR=$(realpath "${SCRIPT_DIR}")/
ROOT_DIR=$(dirname "$0")/../../
ROOT_DIR=$(realpath "${ROOT_DIR}")/

if [[ -f "${ROOT_DIR}scripts/gerbera-shell.sh" ]]; then
  . "${ROOT_DIR}scripts/gerbera-shell.sh"
else
  echo "No script library found at ${ROOT_DIR}scripts/gerbera-shell.sh"
fi

GRB_C_COMPILER=gcc
GRB_CXX_COMPILER=g++

function install-gcc {
  echo "::group::Installing GCC 13"
  sudo zypper install --no-confirm \
    gcc13-c++
  GRB_C_COMPILER=gcc-13
  GRB_CXX_COMPILER=g++-13
  echo "::endgroup::"
}

function install-cmake() {
  echo "::group::Installing CMake"
  sudo zypper install --no-confirm \
    cmake
  echo "::endgroup::"
}

function upload_to_repo() {
  echo "::group::Uploading package"
  sudo zypper install --no-confirm \
     ruby gpg
  #sudo gem install deb-s3

  #echo "${PKG_SIGNING_KEY}" | gpg --import

  #deb-s3 upload \
  #  --bucket=gerbera \
  #  --prefix="${1}" \
  #  --codename="${lsb_codename}" \
  #  --arch="${suse_arch}" \
  #  --lock \
  #  --sign="${PKG_SIGNING_KEY_ID}" \
  #  --access-key-id="${suse_UPLOAD_ACCESS_KEY_ID}" \
  #  --secret-access-key="${suse_UPLOAD_SECRET_ACCESS_KEY}" \
  #  --endpoint="${suse_UPLOAD_ENDPOINT}" \
  #  "${suse_name}"
  echo "::endgroup::"
}

# Pre reqs
zypper ref
sudo zypper install --no-confirm \
   lsb-release sudo wget curl git ca-certificates

# Work around https://github.com/actions/checkout/issues/760
GH_ACTIONS="${GH_ACTIONS-n}"
if [[ "${GH_ACTIONS}" == "y" ]]; then
  git config --global --add safe.directory "$(pwd)"
fi

lsb_codename=$(lsb_release -c --short)
lsb_rel=$(lsb_release -r --short)
if [[ "${lsb_codename}" == "n/a" ]]; then
  lsb_codename="unstable"
fi
lsb_distro=$(lsb_release -i --short)

my_sys=${lsb_codename}
my_upnp=pupnp
if [ $# -gt 0 ]; then
  my_sys=$1
fi
if [ $# -gt 1 ]; then
  my_upnp=$2
fi

echo "Running $0 ${my_sys} ${my_upnp}"

libmagic="file-devel"
libffmpeg="ffmpeg-7-libavformat-devel"
libwavpack="wavpack-devel"
libmysqlclient="libmariadb-devel"

if [[ "${my_sys}" == "HEAD" ]]; then
  libexif=""
  libexiv2=""
  libduktape=""
  libmatroska=""
  libpugixml=""
  ffmpegthumbnailer=""
else
  libexif="libexif-devel"
  libexiv2="libexiv2-devel"
  libpugixml="pugixml-devel"
  libmatroska="libebml-devel libmatroska-devel"
  ffmpegthumbnailer="libffmpegthumbnailer-devel"

  libduktape="duktape"
  libduktape="duktape-devel ${libduktape}"

  echo "Selecting $libduktape for $lsb_distro $lsb_rel"
fi

BUILD_DIR=build-suse

if [[ -f ${SCRIPT_DIR}releases/${lsb_distro}-${lsb_rel}.sh ]]; then
  . ${SCRIPT_DIR}releases/${lsb_distro}-${lsb_rel}.sh
fi
if [[ -f ${SCRIPT_DIR}releases/leap${lsb_rel}.sh ]]; then
  . ${SCRIPT_DIR}releases/leap${lsb_rel}.sh
fi
if [[ -f ${SCRIPT_DIR}releases/${lsb_distro}.sh ]]; then
  . ${SCRIPT_DIR}releases/${lsb_distro}.sh
fi


if [[ ! -d ${BUILD_DIR} ]]; then
  mkdir ${BUILD_DIR}

  install-gcc
  install-cmake

  echo "::group::Installing dependencies"
  sudo zypper install --no-confirm \
      wget autoconf libtool pkg-config \
      ${libffmpeg} \
      ${libduktape} \
      ${libmatroska} \
      ${libexiv2} \
      ${libexif} \
      ${ffmpegthumbnailer} \
      ${libwavpack} \
      ${libmagic} \
      ${libmysqlclient} \
      ${libpugixml} \
      sqlite3-devel \
      libuuid-devel
  echo "::endgroup::"

  if [[ "${my_sys}" == "HEAD" ]]; then
    install-googletest
    install-libexif
    install-libexiv2 head
    install-pugixml
    install-duktape
    install-ffmpegthumbnailer
    install-matroska
  fi

  install-fmt
  install-spdlog
  install-taglib
  if [[ "${my_upnp}" == "npupnp" ]]; then
    install-npupnp
  else
    my_upnp="pupnp"
    install-pupnp
  fi
fi

cd ${BUILD_DIR}

commit_date=$(git log -1 --date=format:"%Y%m%d%H%M%S" --format="%ad")
git_ver=$(git describe --tags | sed 's/\(.*\)-.*/\1/' | sed s/-/+/ | sed s/v//)

# If version contains a + this is a non-tag build so add commit date
suse_version="${git_ver}-${lsb_codename}1"
is_tag=1
if [[ $git_ver == *"+"* ]]; then
  suse_version="${git_ver}~${commit_date}-${lsb_codename}1"
  is_tag=0
fi

suse_arch=$(uname --hardware-platform)
suse_name="gerbera_${suse_version}_${suse_arch}.rpm"

if [[ (! -f ${suse_name}) || "${my_sys}" == "HEAD" ]]; then
  cmake_preset="${my_sys}-${my_upnp}"
  set +euEo pipefail
  ex_preset=$(cmake "${ROOT_DIR}" --list-presets | grep -cs "${cmake_preset}")

  if [[ ${ex_preset} -eq 0 ]]; then
    cmake_preset="release-${my_upnp}"
  fi
  set -euEo pipefail

  cmake "${ROOT_DIR}" --preset="${cmake_preset}" \
    -DCMAKE_CXX_COMPILER=${GRB_CXX_COMPILER} -DCMAKE_C_COMPILER=${GRB_C_COMPILER} \
    -DCMAKE_INSTALL_PREFIX=/usr
  make "-j$(nproc)"

  # https://cmake.org/cmake/help/latest/cpack_gen/rpm.html#cpack_gen:CPack%20RPM%20Generator
  if [[ "${my_sys}" != "HEAD" ]]; then
    cpack -G RPM -R "$suse_version" -D CPACK_RPM_PACKAGE_ARCHITECTURE="$suse_arch"
  fi
else
  printf "OpenSUSE ${suse_name} already built!\n"
fi

if [[ "${my_sys}" != "HEAD" ]]; then
  if [[ "${SUSE_UPLOAD_ENDPOINT:-}" ]]; then
    # Tags only for main repo
    [[ $is_tag == 1 ]] && upload_to_repo suse
    # Git builds go to git
    upload_to_repo suse-git
  else
    printf "Skipping upload due to missing SUSE_UPLOAD_ENDPOINT"
  fi
else
  ctest --output-on-failure
fi
