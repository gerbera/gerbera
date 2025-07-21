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
    JSONCPP="1.7.4"
    CMAKE="3.31.8"

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
    JSONCPP="1.9.6"
    CMAKE="3.31.8"

else

    DUKTAPE="2.7.0"
    EBML="1.4.5"
    EXIV2="v0.28.5"
    EXIF="v0.6.25"
    FFMPEGTHUMBNAILER="2.2.3"
    FMT="11.2.0"
    GOOGLETEST="1.17.0"
    LASTFM="0.4.0"
    MATROSKA="1.7.1"
    NPUPNP="6.2.1"
    PUGIXML="1.15"
    PUPNP="1.14.24"
    SPDLOG="1.15.3"
    WAVPACK="5.8.1"
    TAGLIB="2.1.1"
    JSONCPP="1.9.6"
    CMAKE="4.0.3"

fi
