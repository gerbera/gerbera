/*MT*

    MediaTomb - http://www.mediatomb.cc/

    request_handler.h - this file is part of MediaTomb.

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

/// \file request_handler.h
/// \brief Definition of the RequestHandler class.
/// \todo genych, describe you this request handler...
#ifndef __REQUEST_HANDLER_H__
#define __REQUEST_HANDLER_H__

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "common.h"
#include "context.h"
#include "iohandler/io_handler.h"

// forward declaration
class CdsObject;
class Config;
class Context;
class ContentManager;
class Database;
class Mime;
class Server;

class RequestHandler {
public:
    explicit RequestHandler(std::shared_ptr<ContentManager> content);
    virtual ~RequestHandler() = default;

    /// \brief Returns information about the requested content.
    /// \param filename Requested URL
    /// \param info File_Info structure, quite similar to statbuf.
    virtual void getInfo(const char* filename, UpnpFileInfo* info) = 0;

    /// \brief Prepares the output buffer and calls the process function.
    /// \return IOHandler
    virtual std::unique_ptr<IOHandler> open(const char* filename, enum UpnpOpenFileMode mode) = 0;

    /// \brief Splits the url into a path and parameters string.
    /// Only '?' and '/' separators are allowed, otherwise an exception will
    /// be thrown.
    /// \param url URL that has to be processed
    /// \return pair of path and parameters which reference the input-view of url
    ///
    /// This function splits the url into its path and parameter components.
    /// content/media SEPARATOR object_id=12345&transcode=wav would be transformed to:
    /// path = "content/media"
    /// parameters = "object_id=12345&transcode=wav"
    static std::pair<std::string_view, std::string_view> splitUrl(std::string_view url, char separator);

    static std::string joinUrl(const std::vector<std::string>& components, bool addToEnd = false, std::string_view separator = _URL_PARAM_SEPARATOR);

    static std::map<std::string, std::string> parseParameters(std::string_view filename, std::string_view baseLink);
    std::shared_ptr<CdsObject> getObjectById(const std::map<std::string, std::string>& params) const;

protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<Context> context;
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<Server> server;
};

#endif // __REQUEST_HANDLER_H__
