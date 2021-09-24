/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_ext_handler.h - this file is part of MediaTomb.
    
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

/// \file transcode_ext_handler.h
/// \brief Definition of the TranscodeRequest class.
#ifndef __TRANSCODE_EXTERNAL_HANDLER_H__
#define __TRANSCODE_EXTERNAL_HANDLER_H__

#include <filesystem>
#include <memory>
#include <upnp.h>

#include "common.h"
#include "transcode_handler.h"

class ProcListItem;

class TranscodeExternalHandler : public TranscodeHandler {
public:
    explicit TranscodeExternalHandler(std::shared_ptr<ContentManager> content);

    std::unique_ptr<IOHandler> serveContent(std::shared_ptr<TranscodingProfile> profile,
        std::string location,
        std::shared_ptr<CdsObject> obj,
        std::string range) override;

private:
    std::filesystem::path makeFifo();
    static void checkTranscoder(const std::shared_ptr<TranscodingProfile>& profile);
#ifdef HAVE_CURL
    void openCurlFifo(std::string& location, std::vector<std::shared_ptr<ProcListItem>>& procList);
#endif
#ifdef SOPCAST
    static bool startSopcastConnector(const std::shared_ptr<CdsObject>& obj, std::string& location, std::vector<std::shared_ptr<ProcListItem>>& procList);
#endif
};

#endif // __TRANSCODE_EXTERNAL_HANDLER_H__
