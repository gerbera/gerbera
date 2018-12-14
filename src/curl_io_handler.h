/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    curl_io_handler.h - this file is part of MediaTomb.
    
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

/// \file curl_io_handler.h

#ifdef HAVE_CURL

#ifndef __CURL_IO_HANDLER_H__
#define __CURL_IO_HANDLER_H__

#include <curl/curl.h>
#include <upnp.h>

#include "common.h"
#include "io_handler_buffer_helper.h"

class CurlIOHandler : public IOHandlerBufferHelper {
public:
    CurlIOHandler(zmm::String URL, CURL* curl_handle, size_t bufSize, size_t initialFillSize);

    virtual void open(enum UpnpOpenFileMode mode);
    virtual void close();

private:
    CURL* curl_handle;
    bool external_curl_handle;
    zmm::String URL;
    //off_t bytesCurl;

    static size_t curlCallback(void* ptr, size_t size, size_t nmemb, void* stream);
    virtual void threadProc();
};

#endif // __CURL_IO_HANDLER_H__

#endif //HAVE_CURL
