# Gerbera - https://gerbera.io/
#
# versions.sh - this file is part of Gerbera.
#
# Copyright (C) 2021-2024 Gerbera Contributors
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
    FFMPEGTHUMBNAILER="2.2.0"
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
    EXIV2="v0.28.2"
    FFMPEGTHUMBNAILER="2.2.2"
    FMT="10.2.1"
    GOOGLETEST="1.14.0"
    LASTFM="0.4.0"
    MATROSKA="1.7.1"
    NPUPNP="6.1.2"
    PUGIXML="1.14"
    PUPNP="1.14.19"
    SPDLOG="1.14.1"
    WAVPACK="5.7.0"
    TAGLIB="1.13.1"

fi

UNAME=$(uname)

if [ "$(id -u)" != 0 ]; then
    echo "Please run this script with superuser access!"
    exit 1
fi

function downloadSource()
{
    if [[ ! -f "${tgz_file}" ]]; then
        if [[ $# -gt 1 ]]; then
            set +e
        fi
        wget ${1} -O "${tgz_file}"
        if [[ ! -f "${tgz_file}" || ($? -eq 8 && $# -gt 1) ]]; then
            set -e
            wget ${2} --no-hsts --no-check-certificate -O "${tgz_file}"
        elif [[ $# -gt 1 ]]; then
            exit $?
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

    if [[ -d build ]]; then
        rm -R build
    fi
    mkdir build
    cd build
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
