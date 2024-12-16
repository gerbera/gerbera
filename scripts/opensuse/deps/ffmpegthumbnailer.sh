#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# opensuse/deps/ffmpegthumbnailer.sh - this file is part of Gerbera.
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

if [[ "${lsb_rel}" == "15.4" ]]; then

    sudo zypper install --no-confirm \
            ffmpeg-4 ffmpeg-4-libavfilter-devel ffmpeg-4-libavcodec-devel ffmpeg-4-libavutil-devel \
            libjpeg-devel libpng-devel \
            ffmpeg-4-libavdevice-devel ffmpeg-4-libswresample-devel ffmpeg-4-libavresample-devel

else

    sudo zypper install --no-confirm \
            ffmpeg-7-libavfilter-devel ffmpeg-7-libavcodec-devel ffmpeg-7-libavutil-devel \
            ffmpeg-7-libavdevice-devel ffmpeg-7-libavformat-devel \
            libjpeg8-devel libpng16-devel

fi
