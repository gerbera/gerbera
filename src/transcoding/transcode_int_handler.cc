/*GRB*

    Gerbera - https://gerbera.io/

    transcode_int_handler.cc - this file is part of Gerbera.

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
/// @file transcoding/transcode_int_handler.cc
#define GRB_LOG_FAC GrbLogFacility::transcoding

#include "transcode_int_handler.h"

#ifdef HAVE_FFMPEG

#include "transcode_io_handler.h"

#include "config/config.h"
#include "config/config_val.h"

std::unique_ptr<IOHandler> TranscodeInternalHandler::serveContent(
    const std::shared_ptr<TranscodingProfile>& profile,
    const fs::path& location,
    const std::shared_ptr<CdsObject>& obj,
    const std::string& group,
    const std::string& range)
{
    auto tmpDir = config->getOption(ConfigVal::SERVER_TMPDIR);
    return std::make_unique<TranscodeInternalIOHandler>(location, tmpDir, profile, obj);
}

#endif
