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
/// \brief Definition of the CurlIOHandler class.

#ifndef __CURL_IO_HANDLER_H__
#define __CURL_IO_HANDLER_H__

#ifdef HAVE_CURL

#include <curl/curl.h>
#include <upnp.h>

#include "io_handler_buffer_helper.h"

class Config;

/// \brief Allows the web server to read from a web site.
class CurlIOHandler : public IOHandlerBufferHelper {
public:
    CurlIOHandler(const std::shared_ptr<Config>& config, const std::string& url, std::size_t bufSize, std::size_t initialFillSize);
    ~CurlIOHandler() noexcept override;

    void open(enum UpnpOpenFileMode mode) override;
    void close() override;

private:
    std::string URL;
    bool external_curl_handle;
    CURL* curl_handle;
#if 0
    off_t bytesCurl { 0 };
#endif

    static std::size_t curlCallback(void* ptr, std::size_t size, std::size_t nmemb, CurlIOHandler* ego);
    void threadProc() override;
};

#endif // HAVE_CURL

#endif // __CURL_IO_HANDLER_H__
