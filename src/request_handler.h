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

#include "common.h"
#include "iohandler/io_handler.h"
#include <map>
#include <memory>
#include <string>

// forward declaration
class Config;
class Mime;
class Database;
class ContentManager;
class CdsObject;

class RequestHandler {
public:
    RequestHandler(std::shared_ptr<ContentManager> content);

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
    /// \param path variable where the path portion will be saved
    /// \param parameters variable where the parameters portion will be saved
    ///
    /// This function splits the url into it's path and parameter components.
    /// content/media SEPARATOR object_id=12345&transcode=wav would be transformed to:
    /// path = "content/media"
    /// parameters = "object_id=12345&transcode=wav"
    static void splitUrl(const char* url, char separator, std::string& path, std::string& parameters);

    std::map<std::string, std::string> parseParameters(const char* filename, const char* baseLink);
    std::shared_ptr<CdsObject> getObjectById(std::map<std::string, std::string> params);

    virtual ~RequestHandler() = default;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Mime> mime;
    std::shared_ptr<Database> database;
    std::shared_ptr<ContentManager> content;
};

#endif // __REQUEST_HANDLER_H__
