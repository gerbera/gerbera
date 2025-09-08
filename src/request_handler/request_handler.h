/*MT*

    MediaTomb - http://www.mediatomb.cc/

    request_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file request_handler/request_handler.h
/// @brief Definition of the RequestHandler class.
#ifndef __REQUEST_HANDLER_H__
#define __REQUEST_HANDLER_H__

#include <map>
#include <memory>
#include <upnp.h>
#include <vector>

// forward declaration
class CdsObject;
class Config;
class Context;
class Content;
class Database;
class IOHandler;
class Mime;
class Quirks;
class UpnpXMLBuilder;

#define LINK_FILE_REQUEST_HANDLER "/" CONTENT_MEDIA_HANDLER "/"
#define LINK_URL_REQUEST_HANDLER "/" CONTENT_ONLINE_HANDLER "/"

#define URL_PARAM_TRANSCODE_PROFILE_NAME "pr_name"
#define URL_PARAM_TRANSCODE "tr"
#define URL_VALUE_TRANSCODE_NO_RES_ID "none"
#define URL_VALUE_TRANSCODE "1"
#define URL_OBJECT_ID "object_id"
#define URL_REQUEST_TYPE "req_type"
#define URL_RESOURCE_ID "res_id"

/// @brief base class for all handlers to answer request received via UPnP or web
class RequestHandler {
public:
    explicit RequestHandler(std::shared_ptr<Content> content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder, std::shared_ptr<Quirks> quirks);
    virtual ~RequestHandler();

    /// @brief Returns header information about the requested content.
    /// @param filename Requested URL
    /// @param info \c UpnpFileInfo structure, quite similar to \c statbuf.
    /// @return \c true if there was client info available
    virtual bool getInfo(const char* filename, UpnpFileInfo* info) = 0;

    /// @brief Prepares the output buffer and calls the process function.
    /// @param filename Requested URL
    /// @param quirks allows modifying the content of the response based on the client
    /// @param mode either UPNP_READ or UPNP_WRITE
    /// @return the appropriate IOHandler for the request.
    virtual std::unique_ptr<IOHandler> open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode) = 0;

    std::shared_ptr<CdsObject> loadObject(const std::map<std::string, std::string>& params) const;

protected:
    std::shared_ptr<Content> content;
    std::shared_ptr<Context> context;
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<UpnpXMLBuilder> xmlBuilder;
    std::shared_ptr<Quirks> quirks;
};

#endif // __REQUEST_HANDLER_H__
