# Gerbera - https://gerbera.io/
#
# versions.sh - this file is part of Gerbera.
#
# Copyright (C) 2021-2025 Gerbera Contributors
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

if [[ "${GERBERA_ENV-head}" == "minimum" ]]; then

    DUKTAPE="2.2.1"
    EBML="1.3.9"
    EXIV2="v0.26"
    EXIF="v0.6.22"
    FFMPEGTHUMBNAILER="2.2.0"
    FMT="7.1.3"
    GOOGLETEST="1.10.0"
    LASTFM="0.4.0"
    MATROSKA="1.5.2"
    PUGIXML="1.10"
    PUPNP="1.14.6"
    NPUPNP="4.2.1"
    SPDLOG="1.8.1"
    WAVPACK="5.1.0"
    TAGLIB="1.12"

elif [[ "${GERBERA_ENV-head}" == "default" ]]; then

    DUKTAPE="2.6.0"
    EBML="1.3.9"
    EXIV2="v0.27.7"
    EXIF="v0.6.24"
    FFMPEGTHUMBNAILER="2.2.2"
    FMT="9.1.0"
    GOOGLETEST="1.10.0"
    LASTFM="0.4.0"
    MATROSKA="1.5.2"
    PUGIXML="1.10"
    PUPNP="1.14.17"
    NPUPNP="5.1.2"
    SPDLOG="1.11.0"
    WAVPACK="5.4.0"
    TAGLIB="1.12"

else

    DUKTAPE="2.7.0"
    EBML="1.4.5"
    EXIV2="v0.28.4"
    EXIF="v0.6.25"
    FFMPEGTHUMBNAILER="2.2.3"
    FMT="11.1.3"
    GOOGLETEST="1.16.0"
    LASTFM="0.4.0"
    MATROSKA="1.7.1"
    NPUPNP="6.2.0"
    PUGIXML="1.15"
    PUPNP="1.14.20"
    SPDLOG="1.15.1"
    WAVPACK="5.8.1"
    TAGLIB="2.0.2"

fi

UNAME=$(uname)

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
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
else
    lsb_codename="unknown"
    lsb_distro="unknown"
    lsb_rel="unknown"
fi

function downloadSource()
{
    if [[ ! -f "${tgz_file}" ]]; then
        if [[ $# -gt 1 ]]; then
            set +e
        fi
        wget ${1} --no-hsts --no-check-certificate -O "${tgz_file}"
        WGET_RES=$?
        if [[ ! -f "${tgz_file}" || (${WGET_RES} -eq 8 && $# -gt 1 && "${2}" != "-") ]]; then
            set -e
            wget ${2} --no-hsts --no-check-certificate -O "${tgz_file}"
        elif [[ $# -gt 1 && ${WGET_RES} -gt 0 ]]; then
            exit ${WGET_RES}
        fi
        set -e
    fi

    if [[ -d "${src_dir}" ]]; then
#        exit
        rm -r "${src_dir}"
    fi
    mkdir "${src_dir}"

    tar -xvf "${tgz_file}" --strip-components=1 -C "${src_dir}"

    cd "${src_dir}"

    if [[ $# -gt 2 && -f "${3}" ]]; then
        patch -p1 < ${3}
    fi

    if [[ -d build ]]; then
        rm -R build
    fi
    mkdir build
    cd build
}

function installDeps()
{
    root_dir=$1
    package=$2
    distr=${lsb_distro}
    if [[ "${lsb_distro}" == "ubuntu" ]]; then
        distr="debian"
    elif [[ "${lsb_distro}" == "suse" ]]; then
        distr="opensuse"
    fi
    dep_file="${root_dir}/${distr}/deps/${package}.sh"
    if [[ -f "${dep_file}" ]]; then
        . ${dep_file}
    else
        echo "No dep file ${dep_file}"
    fi
}

function makeInstall()
{
    if command -v nproc >/dev/null 2>&1; then
        make "-j$(nproc)"
    else
        make
    fi
    make install
}

function ldConfig()
{
    if [ "$(uname)" != 'Darwin' ]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
        else
            ID="Linux"
        fi
        if [ "${ID}" == "alpine" ]; then
            ldconfig /
        else
            ldconfig
        fi
    fi
}
