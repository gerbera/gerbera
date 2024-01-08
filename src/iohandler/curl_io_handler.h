/*MT*

    MediaTomb - http://www.mediatomb.cc/

    curl_io_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

class Config;

class CurlIOHandler : public IOHandlerBufferHelper {
public:
    CurlIOHandler(const std::shared_ptr<Config>& config, const std::string& url, CURL* curlHandle, std::size_t bufSize, std::size_t initialFillSize);

    void open(enum UpnpOpenFileMode mode) override;
    void close() override;

private:
    CURL* curl_handle;
    bool external_curl_handle;
    std::string URL;
    // off_t bytesCurl;

    static std::size_t curlCallback(void* ptr, std::size_t size, std::size_t nmemb, CurlIOHandler* ego);
    void threadProc() override;
};

#endif // __CURL_IO_HANDLER_H__

#endif // HAVE_CURL
