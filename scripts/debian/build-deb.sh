#!/usr/bin/env bash

set -Eeuo pipefail

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
  if [[ "$lsb_codename" == "bionic" ]]; then
    sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${lsb_codename} main"
    sudo apt-get update -y
  fi
  sudo apt-get install cmake -y
  echo "::endgroup::"
}

function install-fmt {
  echo "::group::Installing fmt"
  #if [[ "$lsb_codename" == "bionic" || "$lsb_codename" == "buster" || "$lsb_codename" == "hirsute" ]]; then
    sudo bash scripts/install-fmt.sh static
  #else
  #   sudo apt-get install libfmt-dev -y
  #fi
  echo "::endgroup::"
}

function install-spdlog() {
  echo "::group::Installing spdlog"

  #if [[ "$lsb_codename" == "bionic" || "$lsb_codename" == "buster" || "$lsb_codename" == "hirsute" ]]; then
    sudo bash scripts/install-spdlog.sh
  #else
  #  sudo apt-get install libspdlog-dev -y
  #fi
  echo "::endgroup::"
}

function upload_to_artifactory() {

  target_path="pool/main/g/gerbera/$deb_name"
  bintray_url="https://gerbera.jfrog.io/artifactory/$1/$target_path;deb.distribution=$lsb_codename;deb.component=main;deb.architecture=$deb_arch"

  printf "Uploading %s to %s...\n" "$target_path" "$bintray_url"

  curl -H "X-JFrog-Art-Api:$ART_API_KEY" -XPUT  -T "$deb_name" -uian@gerbera.io:"${ART_API_KEY}" "$bintray_url"
}

export DEBIAN_FRONTEND=noninteractive
lsb_codename=$(lsb_release -c --short)
lsb_distro=$(lsb_release -i --short)

install-gcc
install-cmake

libduktape="libduktape205"
if [[ "$lsb_codename" == "bionic" ]]; then
  libduktape="libduktape202"
elif [ "$lsb_codename" == "buster" ]; then
  libduktape="libduktape203"
elif [ "$lsb_codename" == "unstable" -o "$lsb_codename" == "sid" ]; then
  libduktape="libduktape206"
fi

libmysqlclient="libmysqlclient-dev"
if [ "$lsb_distro" == "Debian" ]; then
  libmysqlclient="libmariadb-dev-compat"
fi
if [[ "$lsb_codename" == "hirsute" ]]; then
  libmysqlclient="libmysql++-dev"
fi

if [[ ! -d build-deb ]]; then
  mkdir build-deb

  echo "::group::Installing dependencies"
  sudo apt-get update
  sudo apt-get install -y \
      dpkg-dev \
      systemd \
      wget autoconf libtool pkg-config \
      cmake \
      bsdmainutils \
      duktape-dev \
      libavformat-dev \
      libcurl4-openssl-dev \
      "${libduktape}" \
      libebml-dev \
      libexif-dev \
      libffmpegthumbnailer-dev \
      libmagic-dev \
      libmatroska-dev \
      "${libmysqlclient}" \
      libpugixml-dev \
      libsqlite3-dev \
      libtag1-dev \
      uuid-dev
  sudo apt-get clean
  echo "::endgroup::"
fi

if [[ "$lsb_codename" == "bionic" ]]; then
  # dpkg-dev pulls g++ which changes your GCC symlinks because ubuntu knows better than you
  sudo update-alternatives --set gcc /usr/bin/gcc-8
  sudo update-alternatives --set cpp /usr/bin/cpp-8
fi

install-fmt
install-spdlog

echo "::group::Installing libupnp"
sudo bash scripts/install-pupnp.sh
echo "::endgroup::"

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

if [[ ! -f $deb_name ]]; then
  cmake ../ \
    -DWITH_MAGIC=ON \
    -DWITH_MYSQL=ON \
    -DWITH_CURL=ON \
    -DWITH_JS=ON \
    -DWITH_TAGLIB=ON \
    -DWITH_AVCODEC=ON \
    -DWITH_FFMPEGTHUMBNAILER=ON \
    -DWITH_EXIF=ON \
    -DWITH_LASTFM=OFF \
    -DWITH_SYSTEMD=ON \
    -DWITH_DEBUG=ON \
    -DSTATIC_LIBUPNP=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  make "-j$(nproc)"

  cpack -G DEB -D CPACK_DEBIAN_PACKAGE_VERSION="$deb_version" -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="$deb_arch"
else
  printf "Deb already built!\n"
fi

if [[ "${ART_API_KEY:-}" ]]; then
  # Tags only for main repo
  [[ $is_tag == 1 ]] && upload_to_artifactory debian
  # Git builds go to git
  upload_to_artifactory debian-git
fi
