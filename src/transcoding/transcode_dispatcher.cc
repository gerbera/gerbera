/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcode_dispatcher.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file transcode_dispatcher.cc
#define LOG_FAC log_facility_t::transcoding

#include "transcode_dispatcher.h" // API

#include "cds_objects.h"
#include "iohandler/io_handler.h"
#include "transcode_ext_handler.h"
#include "transcoding.h"

std::unique_ptr<IOHandler> TranscodeDispatcher::serveContent(const std::shared_ptr<TranscodingProfile>& profile,
    const fs::path& location,
    const std::shared_ptr<CdsObject>& obj,
    const std::string& group,
    const std::string& range)
{
    if (!profile)
        throw_std_runtime_error("Transcoding of file {} requested but no profile given ", location.c_str());

    if (profile->getType() == TR_External) {
        auto trExt = std::make_unique<TranscodeExternalHandler>(content);
        return trExt->serveContent(profile, location, obj, group, range);
    }

    throw_std_runtime_error("Unknown transcoding type for profile {}", profile->getName());
}
