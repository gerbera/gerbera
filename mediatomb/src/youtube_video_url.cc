/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_video_url.cc - this file is part of MediaTomb.
    
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

/// \file youtube_video_url.cc
/// \brief Definitions of the Transcoding classes. 

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef YOUTUBE

#include "youtube_video_url.h"
#include "tools.h"

using namespace zmm;

#define YOUTUBE_URL_PARAMS_REGEXP   "player2\\.swf\\?([^\"]+)\""
#define YOUTUBE_URL_LOCATION_REGEXP "\nLocation: (http://[^\n]+)\n"
#define YOUTUBE_URL_WATCH           "http://www.youtube.com/watch?v="
#define YOUTUBE_URL_GET             "http://www.youtube.com/get_video?" 
#define YOUTUBE_PAGESIZE            106496


YouTubeVideoURL::YouTubeVideoURL()
{
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    reVideoURLParams = Ref<RExp>(new RExp());
    reVideoURLParams->compile(_(YOUTUBE_URL_PARAMS_REGEXP));
    redirectLocation = Ref<RExp>(new RExp());
    redirectLocation->compile(_(YOUTUBE_URL_LOCATION_REGEXP));

    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    pid = pthread_self();
}
    
YouTubeVideoURL::~YouTubeVideoURL()
{
    curl_easy_cleanup(curl_handle);
}

String YouTubeVideoURL::getVideoURL(String video_id)
{
    CURLcode res;
    long retcode; 
    char error_buffer[CURL_ERROR_SIZE] = {'\0'};

    if (pid != pthread_self())
        throw _Exception(_("Not allowed to call getVideoURL from different threads!"));

    Ref<StringBuffer> buffer(new StringBuffer(YOUTUBE_PAGESIZE));
   
    video_id = _(YOUTUBE_URL_WATCH) + video_id;

    curl_easy_reset(curl_handle);
//    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl_handle, CURLOPT_URL, video_id.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, YouTubeVideoURL::dl);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)buffer.getPtr());
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }
    
    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &retcode);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    if (retcode != 200)
    {
        log_warning("Failed to get URL for video with id %s,http response %d\n",
                video_id.c_str(), retcode);
        return nil;
    }

    Ref<Matcher> matcher =  reVideoURLParams->matcher(buffer->toString());
    String params;
    if (matcher->next())
    {
        params = matcher->group(1);
    }
    else
    {
        log_warning("Failed to get URL for video with id %s\n",
                    video_id.c_str());
        return nil;
    }

    buffer->clear();
    params = _(YOUTUBE_URL_GET) + params;
    curl_easy_reset(curl_handle);
//    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl_handle, CURLOPT_URL, params.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, YouTubeVideoURL::dl);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)buffer.getPtr());
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    // well, I do not know if there are any other possibilities, but so far
    // it seems that we *always* get a redirect to the actual video resource
    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &retcode);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }
    if (retcode != 303)
    {
        log_error("Unexpected response from YouTube: %d\n", retcode);
        log_error("PLEASE LET US KNOW THAT THIS HAPPENED! contact@mediatomb.cc\n");
        throw _Exception(_("Unexpected YouTube reply: ") + 
                         String::from(retcode));

    }
    matcher = redirectLocation->matcher(buffer->toString());
    if (matcher->next())
    {
        return matcher->group(1);
    }
    return nil;
}

size_t YouTubeVideoURL::dl(void *buf, size_t size, size_t nmemb, void *data)
{
    StringBuffer *buffer = (StringBuffer *)data;
    if (buffer == NULL)
        return 0;

    size_t s = size * nmemb;
    *buffer << String((char *)buf, s);

    return s;
}

#endif//YOUTUBE
