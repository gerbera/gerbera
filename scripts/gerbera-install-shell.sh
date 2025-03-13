# Gerbera - https://gerbera.io/
#
# gerbera-install-shell.sh - this file is part of Gerbera.
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

UNAME=$(uname)

source_files=()
patch_file=""
COMMIT=""

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
        set +e
        fileCount=${#source_files[@]}
        count=0
        WGET_RES=0
        for file in "${source_files[@]}"; do
            count=`expr $count + 1`
            if [[ $count -ge $fileCount ]]; then
                set -e
            fi
            if [[ ! -f "${tgz_file}" || ${WGET_RES} -eq 8 ]]; then
                wget "${file}" --no-hsts --no-check-certificate -O "${tgz_file}"
                WGET_RES=$?
            fi
        done
        if [[ ${WGET_RES} -gt 0 ]]; then
            exit ${WGET_RES}
        fi
        set -e
    fi

    if [[ -d "${src_dir}" ]]; then
        rm -r "${src_dir}"
    fi
    mkdir "${src_dir}"

    tar -xvf "${tgz_file}" --strip-components=1 -C "${src_dir}"

    cd "${src_dir}"

    if [[ "${patch_file}" != "" && -f "${patch_file}" ]]; then
        patch -p1 < "${patch_file}"
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
