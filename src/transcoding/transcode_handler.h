/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcode_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

/// \file transcode_handler.h
/// \brief Definition of the TranscodeRequest class.
#ifndef __TRANSCODE_HANDLER_H__
#define __TRANSCODE_HANDLER_H__

#include <memory>

#include "common.h"
#include "util/grb_fs.h"

// forward declaration
class CdsObject;
class Config;
class ContentManager;
class IOHandler;
class TranscodingProfile;

class TranscodeHandler {
public:
    explicit TranscodeHandler(const std::shared_ptr<ContentManager>& content);
    virtual ~TranscodeHandler() = default;

    virtual std::unique_ptr<IOHandler> serveContent(const std::shared_ptr<TranscodingProfile>& profile,
        const fs::path& location,
        const std::shared_ptr<CdsObject>& obj,
        const std::string& group,
        const std::string& range)
        = 0;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<ContentManager> content;
};

#endif // __TRANSCODE_HANDLER_H__
