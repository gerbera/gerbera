/*MT*

    MediaTomb - http://www.mediatomb.cc/

    url.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// @file util/url.h
/// @brief Definition of the URL class.

#ifndef __URL_H__
#define __URL_H__

#ifdef HAVE_CURL

#include <curl/curl.h>
#include <memory>
#include <string>

/// @brief Handle urls for CURL
class URL {
public:
    /// @brief This is a simplified version of the UpnpFileInfo class as used
    /// in libupnp.
    class Stat {
    public:
        /// @brief Initializes the class with given values, the values
        /// can not be changed afterwards.
        ///
        /// @param url address of the web site
        /// @param size size of the media in bytes
        /// @param mimetype mime type of the media
        Stat(std::string url, curl_off_t size, std::string mimetype)
            : url(std::move(url))
            , size(size)
            , mimetype(std::move(mimetype))
        {
        }

        std::string getURL() const { return url; }
        curl_off_t getSize() const { return size; }
        std::string getMimeType() const { return mimetype; }

    protected:
        std::string url;
        curl_off_t size;
        std::string mimetype;
    };

    /// @brief create url wrapper object
    /// @param url address to open
    /// @param curlHandle an initialized and ready to use curl handle
    URL(std::string url, CURL* curlHandle = nullptr);
    ~URL();

    URL(const URL&) = delete;
    URL& operator=(const URL&) = delete;

    /// @brief downloads either the content or the headers to the buffer.
    ///
    /// This function uses an already initialized curl_handle, the reason
    /// is, that curl might keep the connection open if we do subsequent
    /// requests to the same server.
    ///
    /// @param httpRetcode return code of the http call
    /// @param onlyHeader set true if you only want the header and not the body
    /// @param verbose enable curl verbose option
    /// @param redirect allow redirection
    /// @return downloaded header and content
    std::pair<std::string, std::string> download(long* httpRetcode,
        bool onlyHeader = false,
        bool verbose = false,
        bool redirect = false);

    Stat getInfo();

protected:
    /// @brief This function is installed as a callback for libcurl, when
    /// we download data from a remote site.
    /// @return number of read bytes
    static std::size_t dl(char* buf, std::size_t size, std::size_t nmemb, std::ostringstream* data);

    std::string url;
    CURL* curlHandle;
    bool cleanup { false };
};

#endif // HAVE_CURL

#endif //__URL_H__
