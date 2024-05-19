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

function install-gcc {
  echo "::group::Installing GCC"
  # bionic defaults to gcc-7
  if [[ "$lsb_codename" == "bionic" ]]; then
    sudo apt-get install gcc-8 g++-8 libstdc++-8-dev -y
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
    sudo update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-8 800
  else
    sudo apt-get install g++ -y
  fi
  echo "::endgroup::"
}

function install-cmake() {
  echo "::group::Installing CMake"
  if [[ "$lsb_distro" != "Raspbian" ]]; then
    if [[ "$lsb_codename" == "buster" || "$lsb_codename" == "bionic" || "$lsb_codename" == "focal" ]]; then
      sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
      curl https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
      sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${lsb_codename} main"
      sudo apt-get update -y
    fi
    sudo apt-get install cmake -y
  else
    sudo apt-get install snapd -y
    sudo snap install core
    sudo snap install cmake --classic
  fi
  echo "::endgroup::"
}

function install-fmt {
  echo "::group::Installing fmt"
  sudo bash "${ROOT_DIR}"scripts/install-fmt.sh static
  echo "::endgroup::"
}

function install-spdlog() {
  echo "::group::Installing spdlog"
  sudo bash "${ROOT_DIR}"scripts/install-spdlog.sh static
  echo "::endgroup::"
}

function install-googletest() {
  echo "::group::Installing googletest"
  sudo bash "${ROOT_DIR}"scripts/install-googletest.sh
  echo "::endgroup::"
}

function install-npupnp() {
  echo "::group::Installing libnpupnp"
  sudo bash "${ROOT_DIR}"scripts/install-npupnp.sh
  echo "::endgroup::"
}

function install-pupnp() {
  echo "::group::Installing libupnp"
  sudo bash "${ROOT_DIR}"scripts/install-pupnp.sh
  echo "::endgroup::"
}

function install-taglib() {
  echo "::group::Installing taglib"
  sudo bash "${ROOT_DIR}"scripts/install-taglib.sh
  echo "::endgroup::"
}

function install-ffmpegthumbnailer() {
  echo "::group::Installing ffmpegthumbnailer"
  sudo bash "${ROOT_DIR}"scripts/install-ffmpegthumbnailer.sh
  echo "::endgroup::"
}

function install-pugixml() {
  echo "::group::Installing pugixml"
  sudo bash "${ROOT_DIR}"scripts/install-pugixml.sh
  echo "::endgroup::"
}

function install-duktape() {
  echo "::group::Installing duktape"
  sudo bash "${ROOT_DIR}"scripts/install-duktape.sh
  echo "::endgroup::"
}

function install-libexiv2() {
  echo "::group::Installing libexiv2"
  sudo bash "${ROOT_DIR}"scripts/install-libexiv2.sh static $*
  echo "::endgroup::"
}

function install-matroska() {
  echo "::group::Installing matroska"
  sudo bash "${ROOT_DIR}"scripts/install-ebml.sh
  sudo bash "${ROOT_DIR}"scripts/install-matroska.sh
  echo "::endgroup::"
}

function upload_to_repo() {
  echo "::group::Uploading package"
  sudo apt-get install -y ruby gpg
  sudo gem install deb-s3

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

# Fix time issues
if [[ ! -f /etc/timezone ]]; then
  ln -snf /usr/share/zoneinfo/"$TZ" /etc/localtime && echo "$TZ" > /etc/timezone
fi

# Pre reqs
export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y lsb-release sudo wget curl git ca-certificates

# Work around https://github.com/actions/checkout/issues/760
GH_ACTIONS="${GH_ACTIONS-n}"
if [[ "${GH_ACTIONS}" == "y" ]]; then
  git config --global --add safe.directory "$(pwd)"
fi

lsb_codename=$(lsb_release -c --short)
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

if [[ "${my_sys}" == "HEAD" ]]; then
  libexiv2=""
  libduktape=""
  libmatroska=""
  libpugixml=""
  ffmpegthumbnailer=""
  if [[ "$lsb_codename" == "jammy" ]]; then
    libmatroska="libebml-dev libmatroska-dev"
  fi
else
  libexiv2="libexiv2-dev"
  libpugixml="libpugixml-dev"
  libmatroska="libebml-dev libmatroska-dev"
  ffmpegthumbnailer="libffmpegthumbnailer-dev"

  libduktape="libduktape207"
  if [[ "$lsb_codename" == "bionic" ]]; then
    libduktape="libduktape202"
  elif [[ "$lsb_codename" == "buster" ]]; then
    libduktape="libduktape203"
  elif [[ "$lsb_codename" == "focal" ]]; then
    libduktape="libduktape205"
  fi
  libduktape="duktape-dev ${libduktape}"

  echo "Selecting $libduktape for $lsb_distro $lsb_codename"
fi

libmysqlclient="libmysqlclient-dev"
if [[ "$lsb_distro" == "Debian" || "$lsb_distro" == "Raspbian" ]]; then
  libmysqlclient="libmariadb-dev-compat"
fi
if [[ "$lsb_codename" == "hirsute" || "$lsb_codename" == "impish" || "$lsb_codename" == "jammy" ]]; then
  libmysqlclient="libmysql++-dev"
fi

if [[ ! -d build-deb ]]; then
  mkdir build-deb

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
      libavformat-dev \
      ${libduktape} \
      ${libmatroska} \
      ${libexiv2} \
      libexif-dev \
      ${ffmpegthumbnailer} \
      libwavpack1 libwavpack-dev \
      libmagic-dev \
      ${libmysqlclient} \
      ${libpugixml} \
      libsqlite3-dev \
      uuid-dev
  sudo apt-get clean
  echo "::endgroup::"

  if [[ "$lsb_codename" == "bionic" ]]; then
    # dpkg-dev pulls g++ which changes your GCC symlinks because ubuntu knows better than you
    sudo update-alternatives --set gcc /usr/bin/gcc-8
    sudo update-alternatives --set cpp /usr/bin/cpp-8
  fi

  if [[ "${my_sys}" == "HEAD" ]]; then
    install-googletest
    install-libexiv2 head
    install-pugixml
    install-duktape
    install-ffmpegthumbnailer
    if [[ "$lsb_codename" != "jammy" ]]; then
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

cd build-deb

commit_date=$(git log -1 --date=format:"%Y%m%d%H%M%S" --format="%ad")
git_ver=$(git describe --tags | sed 's/\(.*\)-.*/\1/' | sed s/-/+/ | sed s/v//)

# If version contains a + this is a non-tag build so add commit date
deb_version="${git_ver}-${lsb_codename}1"
is_tag=1
if [[ $git_ver == *"+"* ]]; then
  deb_version="${git_ver}~${commit_date}-${lsb_codename}1"
  is_tag=0
fi

deb_arch=$(dpkg --print-architecture)
deb_name="gerbera_${deb_version}_${deb_arch}.deb"

if [[ (! -f ${deb_name}) || "${my_sys}" == "HEAD" ]]; then
  cmake_preset="${my_sys}-${my_upnp}"
  set +euEo pipefail
  ex_preset=$(cmake "${ROOT_DIR}" --list-presets | grep -cs "${cmake_preset}")

  if [[ ${ex_preset} -eq 0 ]]; then
    cmake_preset="release-${my_upnp}"
  fi
  set -o pipefail

  cmake "${ROOT_DIR}" --preset="${cmake_preset}" \
    -DCMAKE_INSTALL_PREFIX=/usr
  make "-j$(nproc)"

  if [[ "${lsb_distro}" != "Raspbian" ]]; then
    if [[ "${my_sys}" != "HEAD" ]]; then
      cpack -G DEB -D CPACK_DEBIAN_PACKAGE_VERSION="$deb_version" -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="$deb_arch"
    fi
  fi
else
  printf "Deb ${deb_name} already built!\n"
fi

if [[ "${lsb_distro}" != "Raspbian" ]]; then
  if [[ "${my_sys}" != "HEAD" ]]; then
    if [[ "${DEB_UPLOAD_ENDPOINT:-}" ]]; then
      # Tags only for main repo
      [[ $is_tag == 1 ]] && upload_to_repo debian
      # Git builds go to git
      upload_to_repo debian-git
    else
      printf "Skipping upload due to missing DEB_UPLOAD_ENDPOINT"
    fi
  else
    ctest --output-on-failure
  fi
fi
