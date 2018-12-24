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
#include "io_handler.h"

class RequestHandler : public zmm::Object {
public:
    virtual void getInfo(IN const char *filename, OUT UpnpFileInfo *info) = 0;

    virtual zmm::Ref<IOHandler> open(IN const char* filename, IN enum UpnpOpenFileMode mode, IN zmm::String range) = 0;

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
    static void splitUrl(const char *url, char separator, zmm::String &path, zmm::String &parameters);
};

#endif // __REQUEST_HANDLER_H__
