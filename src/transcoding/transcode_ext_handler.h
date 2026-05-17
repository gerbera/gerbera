/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcode_ext_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file transcoding/transcode_ext_handler.h
/// @brief Definition of the TranscodeRequest class.
#ifndef __TRANSCODE_EXTERNAL_HANDLER_H__
#define __TRANSCODE_EXTERNAL_HANDLER_H__

#include "transcode_handler.h"
#include "util/grb_fs.h"

class ProcListItem;

class TranscodeExternalHandler : public TranscodeHandler {
    using TranscodeHandler::TranscodeHandler;

public:
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<TranscodingProfile>& profile,
        const fs::path& location,
        const std::shared_ptr<CdsObject>& obj,
        const std::string& group,
        const std::string& range) override;

    /// @brief Parses a command line, splitting the arguments into an array and
    /// substitutes %in and %out tokens with given strings.
    ///
    /// This function splits a string into array parts, where space is used as the
    /// separator. In addition special %in and %out tokens are replaced by given
    /// strings.
    /// @param line configured command with tokens
    /// @param in replacement for %in
    /// @param out replacement for %out
    /// @param range replacement for %range
    /// @param title replacement for %title
    /// @return vector of strings containing command line items
    static std::vector<std::string> populateCommandLine(
        const std::string& line,
        const std::string& in = "",
        const std::string& out = "",
        const std::string& range = "",
        const std::string& title = "");

private:
    fs::path makeFifo();
    static void checkTranscoder(const std::shared_ptr<TranscodingProfile>& profile);
#ifdef HAVE_CURL
    fs::path openCurlFifo(const fs::path& location, std::vector<ProcListItem>& procList);
#endif
};

#endif // __TRANSCODE_EXTERNAL_HANDLER_H__
