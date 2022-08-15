#!/usr/bin/env bash
set -Eeuo pipefail

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
  if [[ "$lsb_codename" == "bionic" || "$lsb_codename" == "focal" ]]; then
    sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
    curl https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${lsb_codename} main"
    sudo apt-get update -y
  fi
  sudo apt-get install cmake -y
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
  sudo bash "${ROOT_DIR}"scripts/install-libexiv2.sh static
  echo "::endgroup::"
}

function install-matroska() {
  echo "::group::Installing matroska"
  sudo bash "${ROOT_DIR}"scripts/install-ebml.sh
  sudo bash "${ROOT_DIR}"scripts/install-matroska.sh
  echo "::endgroup::"
}

function upload_to_artifactory() {

  target_path="pool/main/g/gerbera/$deb_name"
  bintray_url="https://gerbera.jfrog.io/artifactory/$1/$target_path;deb.distribution=$lsb_codename;deb.component=main;deb.architecture=$deb_arch"

  printf "Uploading %s to %s...\n" "$target_path" "$bintray_url"

  curl -H "X-JFrog-Art-Api:$ART_API_KEY" -XPUT  -T "$deb_name" -uian@gerbera.io:"${ART_API_KEY}" "$bintray_url"
}

# Fix time issues
ln -snf /usr/share/zoneinfo/"$TZ" /etc/localtime && echo "$TZ" > /etc/timezone

# Pre reqs
export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y lsb-release sudo wget curl git ca-certificates

# Work around https://github.com/actions/checkout/issues/760
if ${GITHUB_ACTIONS}; then
  git config --global --add safe.directory "$(pwd)"
fi

lsb_codename=$(lsb_release -c --short)
lsb_distro=$(lsb_release -i --short)

install-gcc
install-cmake
my_sys=${lsb_codename}
if [ $# -gt 0 ]; then
  my_sys=$1
fi

echo "Running $0 ${my_sys}"

if [[ "${my_sys}" == "HEAD" ]]; then
  libexiv2="libexpat1-dev"
  libduktape=""
  libmatroska=""
  libpugixml=""
  ffmpegthumbnailer="ffmpeg libavfilter-dev libavcodec-dev libavutil-dev libavdevice-dev libavresample-dev"
  if [[ "$lsb_codename" == "jammy" ]]; then
    libmatroska="libebml-dev libmatroska-dev"
    ffmpegthumbnailer="libffmpegthumbnailer-dev"
  fi
  BuildType="Debug"
  DoTests="ON"
else
  libexiv2="libexiv2-dev"
  libpugixml="libpugixml-dev"
  libmatroska="libebml-dev libmatroska-dev"
  libduktape="libduktape205"
  ffmpegthumbnailer="libffmpegthumbnailer-dev"
  BuildType="Release"
  DoTests="OFF"
  if [[ "$lsb_codename" == "bionic" ]]; then
    libduktape="libduktape202"
  elif [[ "$lsb_codename" == "buster" ]]; then
    libduktape="libduktape203"
  elif [[ "$lsb_codename" == "bookworm" || "${my_sys}" == "debian:testing" ]]; then
    libduktape="libduktape207"
  elif [[ "$lsb_codename" == "sid" || "$lsb_codename" == "jammy" || "$lsb_codename" == "kinetic" || "${my_sys}" == "debian:unstable" ]]; then
    libduktape="libduktape207"
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

  echo "::group::Installing dependencies"
  sudo apt-get update
  sudo apt-get install -y \
      dpkg-dev \
      systemd \
      build-essential shtool \
      wget autoconf libtool pkg-config \
      cmake \
      bsdmainutils \
      libavformat-dev \
      libcurl4-openssl-dev \
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
fi

if [[ "$lsb_codename" == "bionic" ]]; then
  # dpkg-dev pulls g++ which changes your GCC symlinks because ubuntu knows better than you
  sudo update-alternatives --set gcc /usr/bin/gcc-8
  sudo update-alternatives --set cpp /usr/bin/cpp-8
fi

if [[ "${my_sys}" == "HEAD" ]]; then
  install-googletest
  install-libexiv2
  install-pugixml
  install-duktape
  if [[ "$lsb_codename" != "jammy" ]]; then
    install-matroska
    install-ffmpegthumbnailer
  fi
fi

install-fmt
install-spdlog
install-pupnp
install-taglib

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
  cmake "${ROOT_DIR}" -DWITH_TESTS=${DoTests} \
    -DWITH_MAGIC=ON \
    -DWITH_MYSQL=ON \
    -DWITH_CURL=ON \
    -DWITH_JS=ON \
    -DWITH_TAGLIB=ON \
    -DWITH_AVCODEC=ON \
    -DWITH_FFMPEGTHUMBNAILER=ON \
    -DWITH_EXIF=ON \
    -DWITH_EXIV2=ON \
    -DWITH_WAVPACK=ON \
    -DWITH_LASTFM=OFF \
    -DWITH_SYSTEMD=ON \
    -DWITH_DEBUG=ON \
    -DSTATIC_LIBUPNP=ON \
    -DCMAKE_BUILD_TYPE=${BuildType} \
    -DCMAKE_INSTALL_PREFIX=/usr
  make "-j$(nproc)"

  if [[ "${my_sys}" != "HEAD" ]]; then
    cpack -G DEB -D CPACK_DEBIAN_PACKAGE_VERSION="$deb_version" -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="$deb_arch"
  fi
else
  printf "Deb already built!\n"
fi

if [[ "${my_sys}" != "HEAD" ]]; then
  if [[ "${ART_API_KEY:-}" ]]; then
    # Tags only for main repo
    [[ $is_tag == 1 ]] && upload_to_artifactory debian
    # Git builds go to git
    upload_to_artifactory debian-git
  else
    printf "Skipping upload due to missing ART_API_KEY"
  fi
else
  ctest --output-on-failure
fi
