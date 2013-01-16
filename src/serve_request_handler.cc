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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include "ixml.h"
#include "file_io_handler.h"
#include "dictionary.h"
#include "serve_request_handler.h"
#include "tools.h"


using namespace zmm;
using namespace mxml;

ServeRequestHandler::ServeRequestHandler() : RequestHandler()
{
}

/// \todo clean up the fix for internal items
void ServeRequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{
    struct stat statbuf;
    int ret = 0;
    int len = 0;

    log_debug("got filename: %s\n", filename);
    
    String url_path, parameters;
    split_url(filename, URL_PARAM_SEPARATOR, url_path, parameters);

    log_debug("url_path: %s, parameters: %s\n", url_path.c_str(), parameters.c_str());

    len = (_("/") + SERVER_VIRTUAL_DIR + _("/") + CONTENT_SERVE_HANDLER).length();

    if (len > url_path.length())
    {
        throw _Exception(_("There is something wrong with the link ") + url_path);
    }

    String path = ConfigManager::getInstance()->getOption(CFG_SERVER_SERVEDIR) 
                    + url_path.substring(len, url_path.length()) + 
                    _("/") + parameters;
   
    log_debug("Constructed new path: %s\n", path.c_str());
    
    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw _Exception(_("Failed to stat ") + path);
    }

    if (S_ISREG(statbuf.st_mode)) // we have a regular file
    {
        String mimetype = _(MIMETYPE_DEFAULT);
#ifdef HAVE_MAGIC
        magic_set *ms = NULL;
        ms = magic_open(MAGIC_MIME);
        if (ms != NULL)
        {
            if (magic_load(ms, NULL) == -1)
            {
                log_warning("magic_load: %s\n", magic_error(ms));
                magic_close(ms);
                ms = NULL;
            } 
            else
            {
                /// \TODO we could request the mimetype rex from content manager
                Ref<RExp> reMimetype;
                reMimetype = Ref<RExp>(new RExp());
                reMimetype->compile(_(MIMETYPE_REGEXP));

                String mime = get_mime_type(ms, reMimetype, path);
                if (string_ok(mime))
                    mimetype = mime;

                magic_close(ms);
            }
        }
#endif // HAVE_MAGIC

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
    }
    else
    {
         throw _Exception(_("Not a regular file: ") + path);
    }
}

Ref<IOHandler> ServeRequestHandler::open(IN const char *filename, OUT struct File_Info *info,
                                         IN enum UpnpOpenFileMode mode)
{
    struct stat statbuf;
    int ret = 0;
    int len = 0;

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw _Exception(_("UPNP_WRITE unsupported"));
    
    len = (_("/") + SERVER_VIRTUAL_DIR + _("/") + CONTENT_SERVE_HANDLER).length();
    String url_path, parameters;
    split_url(filename, URL_PARAM_SEPARATOR, url_path, parameters);

    log_debug("url_path: %s, parameters: %s\n", url_path.c_str(), parameters.c_str());

    len = (_("/") + SERVER_VIRTUAL_DIR + _("/") + CONTENT_SERVE_HANDLER).length();

    if (len > url_path.length())
    {
        throw _Exception(_("There is something wrong with the link ") +
                        url_path);
    }

    String path = ConfigManager::getInstance()->getOption(CFG_SERVER_SERVEDIR) 
                  + url_path.substring(len, url_path.length()) + 
                    _("/") + parameters;

    log_debug("Constructed new path: %s\n", path.c_str());
    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw _Exception(_("Failed to stat ") + path);
    }

    if (S_ISREG(statbuf.st_mode)) // we have a regular file
    {
        String mimetype = _(MIMETYPE_DEFAULT);
#ifdef HAVE_MAGIC
        magic_set *ms = NULL;
        ms = magic_open(MAGIC_MIME);
        if (ms != NULL)
        {
            if (magic_load(ms, NULL) == -1)
            {
                log_warning("magic_load: %s\n", magic_error(ms));
                magic_close(ms);
                ms = NULL;
            }
            else
            {
                /// \TODO we could request the mimetype rex from content manager
                Ref<RExp> reMimetype;
                reMimetype = Ref<RExp>(new RExp());
                reMimetype->compile(_(MIMETYPE_REGEXP));

                String mime = get_mime_type(ms, reMimetype, path);
                if (string_ok(mime))
                    mimetype = mime;

                magic_close(ms);
            }
        }
#endif // HAVE_MAGIC

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
    }
    else
    {
         throw _Exception(_("Not a regular file: ") + path);
    }
    Ref<IOHandler> io_handler(new FileIOHandler(path));
    io_handler->open(mode);

    return io_handler;
}
