/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    serve_request_handler.cc - this file is part of MediaTomb.
    
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

/// \file serve_request_handler.cc

#include "serve_request_handler.h"

#include <sys/stat.h>
#include <utility>

#include "config/config_manager.h"
#include "iohandler/file_io_handler.h"
#include "server.h"
#include "util/tools.h"

ServeRequestHandler::ServeRequestHandler(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage)
    : RequestHandler(std::move(config), std::move(storage))
{
}

/// \todo clean up the fix for internal items
void ServeRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    struct stat statbuf;
    int ret = 0;

    log_debug("got filename: {}", filename);

    std::string url_path, parameters;
    splitUrl(filename, URL_PARAM_SEPARATOR, url_path, parameters);

    log_debug("url_path: {}, parameters: {}", url_path.c_str(), parameters.c_str());

    size_t len = (std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER).length();

    if (len > url_path.length()) {
        throw std::runtime_error("There is something wrong with the link " + url_path);
    }

    url_path = urlUnescape(url_path);

    std::string path = config->getOption(CFG_SERVER_SERVEDIR)
        + url_path.substr(len, url_path.length()) + "/" + parameters;

    log_debug("Constructed new path: {}", path.c_str());

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw std::runtime_error("Failed to stat " + path);
    }

    if (S_ISREG(statbuf.st_mode)) // we have a regular file
    {
        std::string mimetype = MIMETYPE_DEFAULT;
#ifdef HAVE_MAGIC
        std::string mime = getMIMETypeFromFile(path);
        if (!mime.empty())
            mimetype = mime;
#endif // HAVE_MAGIC

        UpnpFileInfo_set_FileLength(info, statbuf.st_size);
        UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
        UpnpFileInfo_set_IsDirectory(info, S_ISDIR(statbuf.st_mode));

        if (access(path.c_str(), R_OK) == 0) {
            UpnpFileInfo_set_IsReadable(info, 1);
        } else {
            UpnpFileInfo_set_IsReadable(info, 0);
        }

        UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimetype.c_str()));
    } else {
        throw std::runtime_error("Not a regular file: " + path);
    }
}

std::unique_ptr<IOHandler> ServeRequestHandler::open(const char* filename,
    enum UpnpOpenFileMode mode,
    const std::string& range)
{
    struct stat statbuf;
    int ret = 0;

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw std::runtime_error("UPNP_WRITE unsupported");

    size_t len = (std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER).length();
    std::string url_path, parameters;
    splitUrl(filename, URL_PARAM_SEPARATOR, url_path, parameters);

    log_debug("url_path: {}, parameters: {}", url_path.c_str(), parameters.c_str());

    len = (std::string("/") + SERVER_VIRTUAL_DIR + "/" + CONTENT_SERVE_HANDLER).length();

    if (len > url_path.length()) {
        throw std::runtime_error("There is something wrong with the link " + url_path);
    }

    std::string path = config->getOption(CFG_SERVER_SERVEDIR)
        + url_path.substr(len, url_path.length()) + "/" + parameters;

    log_debug("Constructed new path: {}", path.c_str());
    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw std::runtime_error("Failed to stat " + path);
    }

    if (S_ISREG(statbuf.st_mode)) // we have a regular file
    {
        std::string mimetype = MIMETYPE_DEFAULT;
#ifdef HAVE_MAGIC
        std::string mime = getMIMETypeFromFile(path);
        if (!mime.empty())
            mimetype = mime;
#endif // HAVE_MAGIC

        // FIXME upstream headers
        /*
        info->file_length = statbuf.st_size;
        info->last_modified = statbuf.st_mtime;
        info->is_directory = S_ISDIR(statbuf.st_mode);

        if (access(path.c_str(), R_OK) == 0)
        {
            info->is_readable = 1;
        }
        else
        {
            info->is_readable = 0;
        }


        info->content_type = ixmlCloneDOMString(mimetype.c_str());
        */
    } else {
        throw std::runtime_error("Not a regular file: " + path);
    }

    auto io_handler = std::make_unique<FileIOHandler>(path);
    io_handler->open(mode);
    return io_handler;
}
