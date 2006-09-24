/*  request_handler.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file request_handler.h
/// \brief Definition of the RequestHandler class.
/// \todo genych, opishi ty etot request handler...
#ifndef __REQUEST_HANDLER_H__
#define __REQUEST_HANDLER_H__

#include "common.h"
#include "io_handler.h"

class RequestHandler : public zmm::Object
{
public:
    RequestHandler();
    virtual void get_info(IN const char *filename, OUT struct File_Info *info);
    virtual zmm::Ref<IOHandler> open(IN const char *filename, IN enum UpnpOpenFileMode mode);

    /// \brief Splits the url into a path and parameters string.
    /// \param url URL that has to be processed
    /// \param path variable where the path portion will be saved
    /// \param parameters variable where the parameters portion will be saved
    ///
    /// This function splits the url into it's path and parameter components.
    /// content/media SEPARATOR object_id=12345&transcode=wav would be transformed to: 
    /// path = "content/media"
    /// parameters = "object_id=12345&transcode=wav"
    static void split_url(const char *url, char separator, zmm::String &path, zmm::String &parameters);
};


#endif // __REQUEST_HANDLER_H__

