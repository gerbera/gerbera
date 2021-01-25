#!/usr/bin/env bash

set -Eeuo pipefail

function install-gcc {
  if [[ "$lsb_codename" == "bionic" ]]; then
    sudo apt-get install gcc-8 g++-8 libstdc++-8-dev -y
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
    sudo update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-8 800
  else
    sudo apt-get install g++ -y
  fi
}

function install-cmake() {
  if [[ "$lsb_codename" == "bionic" ]]; then
    sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
    sudo apt-add-repository "deb https://apt.kitware.com/ubuntu/ ${lsb_codename} main"
    sudo apt-get update -y
  fi
  sudo apt-get install cmake -y
}

function install-fmt {
  if [[ "$lsb_codename" == "bionic" || $lsb_codename == "buster" ]]; then
    git clone https://github.com/fmtlib/fmt
    pushd fmt
    git checkout 7.0.0
    popd
    mkdir fmt-build
    pushd fmt-build
    cmake ../fmt -DBUILD_SHARED_LIBS=NO -DFMT_TEST=NO
    make
    sudo make install
    popd
  else
     sudo apt-get install libfmt-dev -y
  fi
}

function install-spdlog() {
  if [[ "$lsb_codename" == "bionic" || $lsb_codename == "buster" ]]; then
    sudo bash scripts/install-spdlog.sh
  else
    sudo apt-get install libspdlog-dev -y
  fi
}

function upload_to_bintray() {
  target_path="pool/main/g/gerbera/$deb_name"
  bintray_url="https://api.bintray.com/content/gerbera/$1/gerbera/$deb_version/$target_path"

  printf "Uploading %s to %s...\n" "$target_path" "$bintray_url"

  curl \
    -H "X-Bintray-Override: 1" \
    -H "X-Bintray-Publish: 1" \
    -H "X-Bintray-Debian-Architecture: $deb_arch" \
    -H "X-Bintray-Debian-Distribution: $lsb_codename" \
    -H "X-Bintray-Debian-Component: main" \
    -T "$deb_name" -uwhyman:"${BINTRAY_API_KEY}" "$bintray_url"
}

export DEBIAN_FRONTEND=noninteractive
lsb_codename=$(lsb_release -c --short)
lsb_distro=$(lsb_release -i --short)

install-gcc
install-cmake

libduktape="libduktape205"
if [[ "$lsb_codename" == "bionic" ]]; then
  libduktape="libduktape202"
elif [ $lsb_codename == "buster" ]; then
  libduktape="libduktape203"
fi

libmysqlclient="libmysqlclient-dev"
if [ "$lsb_distro" == "Debian" ]; then
    libmysqlclient="libmariadb-dev-compat"
fi

if [[ ! -d build-deb ]]; then
  mkdir build-deb

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
fi

if [[ "$lsb_codename" == "bionic" ]]; then
  # dpkg-dev pulls g++ which changes your GCC symlinks because ubuntu knows better than you
  sudo update-alternatives --set gcc /usr/bin/gcc-8
  sudo update-alternatives --set cpp /usr/bin/cpp-8
fi

install-fmt
install-spdlog

sudo bash scripts/install-pupnp.sh

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
  cmake ../ -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
      -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 \
      -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=1 -DWITH_DEBUG=OFF \
      -DSTATIC_LIBUPNP=ON -DCMAKE_BUILD_TYPE=Release
  make "-j$(nproc)"

  cpack -G DEB -D CPACK_DEBIAN_PACKAGE_VERSION="$deb_version" -D CPACK_DEBIAN_PACKAGE_ARCHITECTURE="$deb_arch"
else
  printf "Deb already built!\n"
fi

if [[ "${BINTRAY_API_KEY:-}" ]]; then
  # Tags only for main repo
  [[ $is_tag == 1 ]] && upload_to_bintray gerbera
  # Git builds go to git
  upload_to_bintray gerbera-git
fi
