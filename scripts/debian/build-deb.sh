#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# build-deb.sh - this file is part of Gerbera.
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
  sudo apt-get install g++ -y
  echo "::endgroup::"
}

function install-cmake() {
  echo "::group::Installing CMake"
  sudo apt-get install cmake -y
  echo "::endgroup::"
}

function set-libraries-dist() {
  echo "No ${lsb_distro} Libraries"
}

function set-libraries-rel() {
  echo "No ${lsb_codename} Libraries"
}

function set-libraries() {
  libffmpeg="libavformat-dev"
  libexif="libexif-dev"
  libexiv2="libexiv2-dev"
  libpugixml="libpugixml-dev"
  libmatroska="libebml-dev libmatroska-dev"
  ffmpegthumbnailer="libffmpegthumbnailer-dev"
  libduktape="duktape-dev libduktape207"
  libmysqlclient="libmysqlclient-dev"
  libmagic="libmagic-dev"
  libwavpack="libwavpack1 libwavpack-dev"
}

function install-deb-s3() {
  sudo apt-get install -y ruby gpg
  sudo gem install aws-sdk-core -v 3.215.0
  sudo gem install aws-sdk-s3 -v 1.177.0
  sudo gem install deb-s3
}

function upload_to_repo() {
  echo "::group::Uploading package"
  install-deb-s3

  echo "${PKG_SIGNING_KEY}" | gpg --import

  deb-s3 upload \
    --bucket=gerbera \
    --prefix="${1}" \
    --codename="${lsb_codename}" \
    --arch="${deb_arch}" \
    --lock \
    --sign="${PKG_SIGNING_KEY_ID}" \
    --access-key-id="${DEB_UPLOAD_ACCESS_KEY_ID}" \
    --secret-access-key="${DEB_UPLOAD_SECRET_ACCESS_KEY}" \
    --endpoint="${DEB_UPLOAD_ENDPOINT}" \
    "${deb_name}"
  echo "::endgroup::"
}

echo "::group::Preparing Libraries"

# Fix time issues
if [[ ! -f /etc/timezone ]]; then
  ln -snf /usr/share/zoneinfo/"$TZ" /etc/localtime && echo "$TZ" > /etc/timezone
fi

# Pre reqs
export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y lsb-release sudo wget curl git ca-certificates

# Work around https://github.com/actions/checkout/issues/760
GH_ACTIONS="${GH_ACTIONS:-n}"
if [[ "${GH_ACTIONS}" == "y" ]]; then
  git config --global --add safe.directory "$(pwd)"
fi

lsb_codename=$(lsb_release -c --short)
if [[ "${lsb_codename}" == "n/a" ]]; then
  lsb_codename="unstable"
fi
lsb_distro=$(lsb_release -i --short)
deb_arch=$(dpkg --print-architecture)

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

echo "Running $0 ${my_sys} ${my_upnp} for ${lsb_distro}-${lsb_codename}-${deb_arch}"

set-libraries
set-libraries-dist
set-libraries-rel

echo "Selecting ${libduktape} for ${lsb_distro} ${lsb_codename}"

if [[ "${my_sys}" == "HEAD" ]]; then
  libexif=""
  libexiv2=""
  libduktape=""
  libpugixml=""
  ffmpegthumbnailer=""
  if [[ "${lsb_codename}" != "jammy" ]]; then
    libmatroska=""
  fi
fi

echo "::endgroup::"

BUILD_DIR="build-deb"

if [[ ! -d "${BUILD_DIR}" ]]; then
  mkdir "${BUILD_DIR}"

  install-gcc
  install-cmake

  echo "::group::Installing dependencies"
  sudo apt-get update
  sudo apt-get install -y \
      dpkg-dev \
      systemd \
      build-essential shtool \
      wget autoconf libtool pkg-config \
      bsdmainutils \
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
      libsqlite3-dev \
      uuid-dev
  sudo apt-get clean

  if [[ "${lsb_codename}" == "bionic" ]]; then
    # dpkg-dev pulls g++ which changes your GCC symlinks because ubuntu knows better than you
    sudo update-alternatives --set gcc /usr/bin/gcc-8
    sudo update-alternatives --set cpp /usr/bin/cpp-8
  fi
  echo "::endgroup::"

  if [[ "${my_sys}" == "HEAD" ]]; then
    install-googletest
    install-libexif
    install-libexiv2 head
    install-pugixml
    install-duktape
    install-ffmpegthumbnailer
    if [[ "${lsb_codename}" != "jammy" ]]; then
      install-matroska
    fi
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

cd "${BUILD_DIR}"

commit_date=$(git log -1 --date=format:"%Y%m%d%H%M%S" --format="%ad")
git_ver=$(git describe --tags | sed 's/\(.*\)-.*/\1/' | sed s/-/+/ | sed s/v//)

# If version contains a + this is a non-tag build so add commit date
deb_version="${git_ver}-${lsb_codename}1"
is_tag=1
if [[ ${git_ver} == *"+"* ]]; then
  deb_version="${git_ver}~${commit_date}-${lsb_codename}1"
  is_tag=0
fi
if [[ ${GH_EVENT:-} == "pull_request" ]]; then
  is_tag=2
fi

deb_name="gerbera_${deb_version}_${deb_arch}.deb"

echo "::group::SystemD build support"
set +e
SYSTEMD_PATH=$(pkg-config --variable=systemdsystemunitdir systemd)
WITH_SYSTEMD="ON"
if [[ -z "${SYSTEMD_PATH}" ]]; then
  sudo apt-get install -y \
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

if [[ (! -f ${deb_name}) || "${my_sys}" == "HEAD" ]]; then
  cmake_preset="${my_sys}-${my_upnp}"
  set +euEo pipefail
  ex_preset=$(cmake "${ROOT_DIR}" --list-presets | grep -cs "${cmake_preset}")

  if [[ ${ex_preset} -eq 0 ]]; then
    cmake_preset="release-${my_upnp}"
  fi
  set -euEo pipefail

  echo "::group::Building gerbera"
  cmake "${ROOT_DIR}" --preset="${cmake_preset}" \
    -DWITH_SYSTEMD=${WITH_SYSTEMD} \
    -DBUILD_CHANGELOG=ON \
    -DCMAKE_INSTALL_PREFIX=/usr
  make "-j$(nproc)"

  if [[ "${lsb_distro}" != "Raspbian" ]]; then
    if [[ "${my_sys}" != "HEAD" ]]; then
      cpack -G DEB \
            -D CPACK_DEBIAN_PACKAGE_VERSION="${deb_version}" \
            -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="${deb_arch}" \
            -D CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA="${ROOT_DIR}/scripts/debian/postinst;${ROOT_DIR}/${BUILD_DIR}/changelog.gz"
    fi
  fi
  echo "::endgroup::"
else
  printf "Deb ${deb_name} already built!\n"
fi

if [[ "${lsb_distro}" != "Raspbian" && "${lsb_codename}_${deb_arch}" != "focal_armhf" ]]; then
  if [[ "${my_sys}" != "HEAD" ]]; then
    if [[ "${DEB_UPLOAD_ENDPOINT:-}" ]]; then
      # Tags only for main repo
      [[ $is_tag == 1 ]] && upload_to_repo debian
      # Git builds go to git
      [[ $is_tag == 0 ]] && upload_to_repo debian-git
      exit 0
    else
      printf "Skipping upload due to missing DEB_UPLOAD_ENDPOINT"
    fi
  else
    echo "::group::Testing gerbera"
    ctest --output-on-failure
    echo "::endgroup::"
  fi
fi
