/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    get_url.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file get_url.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_CURL

#include <pthread.h>
#include "get_url.h"

using namespace zmm;

GetURL::GetURL(size_t buffer_hint)
{
    this->buffer_hint = buffer_hint;
}

Ref<StringBuffer> GetURL::download(CURL *curl_handle, String URL, 
                                   long *HTTP_retcode, bool only_header, 
                                   bool verbose)
{
    CURLcode res;
    char error_buffer[CURL_ERROR_SIZE] = {'\0'};

    if (curl_handle == NULL)
        throw _Exception(_("Invalid curl handle!\n"));

    Ref<StringBuffer> buffer(new StringBuffer(buffer_hint));

    curl_easy_reset(curl_handle);
    if (verbose)
        curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);

    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

    if (only_header)
    {
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, GetURL::dl);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, 
                         (void *)buffer.getPtr());
    }
    else
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, GetURL::dl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, 
                         (void *)buffer.getPtr());
    }
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, HTTP_retcode);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    return buffer;
}

size_t GetURL::dl(void *buf, size_t size, size_t nmemb, void *data)
{
    StringBuffer *buffer = (StringBuffer *)data;
    if (buffer == NULL)
        return 0;

    size_t s = size * nmemb;
    *buffer << String((char *)buf, s);

    return s;
}

#endif//HAVE_CURL

