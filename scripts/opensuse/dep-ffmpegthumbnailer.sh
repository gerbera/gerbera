#!/usr/bin/env bash
# Gerbera - https://gerbera.io/
#
# dep-ffmpegthumbnailer.sh - this file is part of Gerbera.
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

sudo zypper install --no-confirm \
       ffmpeg-4 ffmpeg-4-libavfilter-devel ffmpeg-4-libavcodec-devel ffmpeg-4-libavutil-devel \
       libjpeg-devel libpng-devel \
       ffmpeg-4-libavdevice-devel ffmpeg-4-libswresample-devel ffmpeg-4-libavresample-devel
