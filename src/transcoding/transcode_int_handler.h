/*GRB*

    Gerbera - https://gerbera.io/

    transcode_int_handler.h - this file is part of Gerbera.

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
/// @file transcoding/transcode_int_handler.h
/// @brief Definition of the TranscodeHandler class for internal transcoding.
#ifndef __TRANSCODE_INTERNAL_HANDLER_H__
#define __TRANSCODE_INTERNAL_HANDLER_H__
#ifdef HAVE_FFMPEG

#include "transcode_handler.h"

class TranscodeInternalHandler : public TranscodeHandler {
    using TranscodeHandler::TranscodeHandler;

public:
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<TranscodingProfile>& profile,
        const fs::path& location,
        const std::shared_ptr<CdsObject>& obj,
        const std::string& group,
        const std::string& range) override;
};

#endif // HAVE_FFMPEG
#endif // __TRANSCODE_INTERNAL_HANDLER_H__
