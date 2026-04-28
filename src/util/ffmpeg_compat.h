/*GRB*

    Gerbera - https://gerbera.io/

    ffmpeg_compat.h - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/
/// @file util/ffmpeg_compat.h
#ifndef __FFMPEG_COMPAT_H__
#define __FFMPEG_COMPAT_H__
#ifdef HAVE_FFMPEG

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/error.h>

} // extern "C"

// Thanks https://ffmpeg.org/pipermail/ffmpeg-user/2021-June/053077.html
// Thanks https://github.com/joncampbell123/composite-video-simulator/issues/5
#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum)
{
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif // av_err2str

#ifdef HAVE_AVSTREAM_CODECPAR
#define as_codecpar(s) s->codecpar
#else
#define as_codecpar(s) s->codec
#endif

#endif // HAVE_FFMPEG
#endif // __FFMPEG_COMPAT_H__
