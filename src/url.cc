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

#include <pthread.h>
#include "common.h"
#include "rexp.h"
#include "url.h"
#include "tools.h"
#include "config_manager.h"

using namespace zmm;

URL::URL(size_t buffer_hint)
{
    this->buffer_hint = buffer_hint;
}

Ref<StringBuffer> URL::download(String URL, long *HTTP_retcode, 
                                CURL *curl_handle, bool only_header, 
                                bool verbose, bool redirect)
{
    CURLcode res;
    bool cleanup = false;
    char error_buffer[CURL_ERROR_SIZE] = {'\0'};

    if (curl_handle == NULL)
    {
        curl_handle = curl_easy_init();
        cleanup = true;
        if (curl_handle == NULL)
            throw _Exception(_("Invalid curl handle!\n"));
    }

    Ref<StringBuffer> buffer(new StringBuffer(buffer_hint));

    curl_easy_reset(curl_handle);
    
    if (verbose)
    {
        bool logEnabled;
#ifdef TOMBDEBUG
        logEnabled = !ConfigManager::isDebugLogging();
#else
        logEnabled = ConfigManager::isDebugLogging();
#endif
        if (logEnabled)
            curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
    }
    // some web sites send unexpected stuff, seems they need a user agent
    // that they know...
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, 
                     "Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.1.6) Gecko/20091216 Fedora/3.5.6-1.fc12 Firefox/3.5.6");
    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 20); // seconds

    /// \todo it would be a good idea to allow both variants, i.e. retrieve
    /// the headers and data in one go when needed
    if (only_header)
    {
        curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, URL::dl);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, 
                         (void *)buffer.getPtr());
    }
    else
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, URL::dl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, 
                         (void *)buffer.getPtr());
    }

    if (redirect)
    {
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, -1);
    }

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(error_buffer);
    }

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, HTTP_retcode);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(error_buffer);
    }

    if (cleanup)
        curl_easy_cleanup(curl_handle);

    return buffer;
}

Ref<URL::Stat> URL::getInfo(String URL, CURL *curl_handle)
{
    long retcode;
    bool cleanup = false;
    CURLcode res;
    double cl;
    char *ct;
    char *c_url;
    char error_buffer[CURL_ERROR_SIZE] = {'\0'};
    String mt;
    String used_url;

    if (curl_handle == NULL)
    {
        curl_handle = curl_easy_init();
        cleanup = true;
        if (curl_handle == NULL)
            throw _Exception(_("Invalid curl handle!\n"));
    }

    Ref<StringBuffer> buffer = download(URL, &retcode, curl_handle, true, true, true);
    if (retcode != 200)
    {
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(_("Error retrieving information from ") +
                          URL + _(" HTTP return code: ") + 
                          String::from(retcode));
    }
/*    
    Ref<RExp> getMT(new RExp());
    
    try
    {
        getMT->compile(_("\nContent-Type: ([^\n]+)\n"), REG_ICASE);
    }
    catch (Exception ex)
    {
        if (cleanup)
            curl_easy_cleanup(curl_handle);

        throw ex;
    }

    Ref<Matcher> matcher = getMT->matcher(buffer->toString());
    String mt;
    if (matcher->next())
        mt = trim_string(matcher->group(1));
    else
        mt = _(MIMETYPE_DEFAULT);

    log_debug("Extracted content type: %s\n", mt.c_str());

    Ref<RExp> getCL(new RExp());

    try
    {
        getCL->compile(_("\nContent-Length: ([^\n]+)\n"), REG_ICASE);
    }
    catch (Exception ex)
    {
        if (cleanup)
            curl_easy_cleanup(curl_handle);

        throw ex;
    }

    matcher = getCL->matcher(buffer->toString());
    off_t cl;
    if (matcher->next())
        cl = trim_string(matcher->group(1)).toOFF_T();
    else
        cl = -1;
*/
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(error_buffer);
    }
 
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(error_buffer);
    }

    if (ct == NULL)
        mt = _(MIMETYPE_DEFAULT);
    else
        mt = ct;
    
    log_debug("Extracted content length: %lld\n", (long long)cl);

    res = curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &c_url);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        if (cleanup)
            curl_easy_cleanup(curl_handle);
        throw _Exception(error_buffer);
    }

    if (c_url == NULL)
        used_url = URL;
    else
        used_url = c_url;

    Ref<Stat> st(new Stat(used_url, (off_t)cl, mt));

    if (cleanup)
        curl_easy_cleanup(curl_handle);

    return st;
}


size_t URL::dl(void *buf, size_t size, size_t nmemb, void *data)
{
    StringBuffer *buffer = (StringBuffer *)data;
    if (buffer == NULL)
        return 0;

    size_t s = size * nmemb;
    *buffer << String((char *)buf, s);

    return s;
}

#endif//HAVE_CURL
