/*MT*

    MediaTomb - http://www.mediatomb.cc/

    url.h - this file is part of MediaTomb.

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

/// \file url.h
/// \brief Definition of the URLL class.

#ifdef HAVE_CURL

#ifndef __URL_H__
#define __URL_H__

#include <curl/curl.h>
#include <memory>

#include "common.h"

class URL {
public:
    /// \brief This is a simplified version of the File_Info class as used
    /// in libupnp.
    class Stat {
    public:
        /// \brief Initializes the class with given values, the values
        /// can not be changed afterwards.
        ///
        /// \param size size of the media in bytes
        /// \param mimetype mime type of the media
        Stat(std::string url, off_t size, std::string mimetype)
            : url(std::move(url))
            , size(size)
            , mimetype(std::move(mimetype))
        {
        }

        std::string getURL() const { return url; }
        off_t getSize() const { return size; }
        std::string getMimeType() const { return mimetype; }

    protected:
        std::string url;
        off_t size;
        std::string mimetype;
    };

    /// \brief downloads either the content or the headers to the buffer.
    ///
    /// This function uses an already initialized curl_handle, the reason
    /// is, that curl might keep the connection open if we do subsequent
    /// requests to the same server.
    ///
    /// \param curl_handle an initialized and ready to use curl handle
    /// \param URL
    /// \param only_header set true if you only want the header and not the
    /// body
    /// \param vebose enable curl verbose option
    static std::string download(const std::string& url,
        long* httpRetcode,
        CURL* curlHandle = nullptr,
        bool onlyHeader = false,
        bool verbose = false,
        bool redirect = false);

    static std::unique_ptr<Stat> getInfo(const std::string& url, CURL* curlHandle = nullptr);

protected:
    /// \brief This function is installed as a callback for libcurl, when
    /// we download data from a remote site.
    static std::size_t dl(char* buf, std::size_t size, std::size_t nmemb, std::ostringstream* data);
};

#endif //__URL_H__

#endif // HAVE_CURL
