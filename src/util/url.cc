/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    url.cc - this file is part of MediaTomb.
    
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

/// \file url.cc

#ifdef HAVE_CURL
#include "url.h" // API

#include <sstream>

#include "config/config_manager.h"

std::string URL::download(const std::string& url, long* httpRetcode,
    CURL* curlHandle, bool onlyHeader,
    bool verbose, bool redirect)
{
    CURLcode res;
    bool cleanup = false;
    char errorBuffer[CURL_ERROR_SIZE] = { '\0' };

    if (!curlHandle) {
        curlHandle = curl_easy_init();
        cleanup = true;
        if (!curlHandle)
            throw_std_runtime_error("Invalid curl handle");
    }

    std::ostringstream buffer;

    curl_easy_reset(curlHandle);

    if (verbose) {
        bool logEnabled;
#ifdef TOMBDEBUG
        logEnabled = !ConfigManager::isDebugLogging();
#else
        logEnabled = ConfigManager::isDebugLogging();
#endif
        if (logEnabled)
            curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1);
    }
    // some web sites send unexpected stuff, seems they need a user agent
    // that they know...
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT,
        "Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.1.6) Gecko/20091216 Fedora/3.5.6-1.fc12 Firefox/3.5.6");
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 20); // seconds

    /// \todo it would be a good idea to allow both variants, i.e. retrieve
    /// the headers and data in one go when needed
    if (onlyHeader) {
        curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, URL::dl);
        curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, &buffer);
    } else {
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, URL::dl);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &buffer);
    }

    if (redirect) {
        curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curlHandle, CURLOPT_MAXREDIRS, -1);
    }

    res = curl_easy_perform(curlHandle);
    if (res != CURLE_OK) {
        log_error("{}", errorBuffer);
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error(errorBuffer);
    }

    res = curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, httpRetcode);
    if (res != CURLE_OK) {
        log_error("{}", errorBuffer);
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error(errorBuffer);
    }

    if (cleanup)
        curl_easy_cleanup(curlHandle);

    return buffer.str();
}

std::unique_ptr<URL::Stat> URL::getInfo(const std::string& url, CURL* curlHandle)
{
    long retcode;
    bool cleanup = false;
    CURLcode res;
    curl_off_t cl;
    char* ct;
    char* cUrl;
    char errorBuffer[CURL_ERROR_SIZE] = { '\0' };

    if (!curlHandle) {
        curlHandle = curl_easy_init();
        cleanup = true;
        if (!curlHandle)
            throw_std_runtime_error("Invalid curl handle");
    }

    download(url, &retcode, curlHandle, true, true, true);
    if (retcode != 200) {
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error("Error retrieving information from {} - HTTP return code: {}", url.c_str(), retcode);
    }

    res = curl_easy_getinfo(curlHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
    if (res != CURLE_OK) {
        log_error("{}", errorBuffer);
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error(errorBuffer);
    }

    res = curl_easy_getinfo(curlHandle, CURLINFO_CONTENT_TYPE, &ct);
    if (res != CURLE_OK) {
        log_error("{}", errorBuffer);
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error(errorBuffer);
    }

    std::string mt = ct ? ct : MIMETYPE_DEFAULT;
    log_debug("Extracted content length: {}", cl);

    res = curl_easy_getinfo(curlHandle, CURLINFO_EFFECTIVE_URL, &cUrl);
    if (res != CURLE_OK) {
        log_error("{}", errorBuffer);
        if (cleanup)
            curl_easy_cleanup(curlHandle);
        throw_std_runtime_error(errorBuffer);
    }

    std::string usedUrl = cUrl ? cUrl : url;

    if (cleanup)
        curl_easy_cleanup(curlHandle);

    return std::make_unique<Stat>(usedUrl, off_t(cl), mt);
}

std::size_t URL::dl(char* buf, std::size_t size, std::size_t nmemb, std::ostringstream* data)
{
    auto s = size * nmemb;
    *data << std::string(buf, s);

    return s;
}

#endif //HAVE_CURL
