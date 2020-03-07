/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_request_handler.h - this file is part of MediaTomb.
    
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

/// \file file_request_handler.h
/// \brief Definition of the FileRequestHandler class.
#ifndef __FILE_REQUEST_HANDLER_H__
#define __FILE_REQUEST_HANDLER_H__

#include "common.h"
#include "request_handler.h"
#include "upnp_xml.h"
#include <memory>

// forward declaration
class ConfigManager;
class ContentManager;
class UpdateManager;
namespace web {
class SessionManager;
}

class FileRequestHandler : public RequestHandler {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<UpdateManager> updateManager;
    std::shared_ptr<web::SessionManager> sessionManager;
    UpnpXMLBuilder* xmlBuilder;

public:
    explicit FileRequestHandler(std::shared_ptr<ConfigManager> config,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content,
        std::shared_ptr<UpdateManager> updateManager, std::shared_ptr<web::SessionManager> sessionManager,
        UpnpXMLBuilder* xmlBuilder);

    void getInfo(const char* filename, UpnpFileInfo* info) override;
    std::unique_ptr<IOHandler> open(
        const char* filename,
        enum UpnpOpenFileMode mode,
        const std::string& range) override;
};

#endif // __FILE_REQUEST_HANDLER_H__
