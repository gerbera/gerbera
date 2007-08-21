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
#include "get_url.h"

using namespace zmm;

#define YOUTUBE_URL_PARAMS_REGEXP   "player2\\.swf\\?([^\"]+)\""
#define YOUTUBE_URL_LOCATION_REGEXP "\nLocation: (http://[^\n]+)\n"
#define YOUTUBE_URL_WATCH           "http://www.youtube.com/watch?v="
#define YOUTUBE_URL_GET             "http://www.youtube.com/get_video?" 


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
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

String YouTubeVideoURL::getVideoURL(String video_id)
{
    long retcode; 
    String flv_location;

    video_id = _(YOUTUBE_URL_WATCH) + video_id;

    Ref<GetURL> url(new GetURL(YOUTUBE_PAGESIZE));

    Ref<StringBuffer> buffer = url->download(curl_handle, video_id, &retcode);
    if (retcode != 200)
    {
        log_error("Failed to get URL for video with id %s, http response %ld\n", video_id.c_str(), retcode);
        return nil;
    }

    Ref<Matcher> matcher =  reVideoURLParams->matcher(buffer->toString());
    String params;
    if (matcher->next())
    {
        params = trim_string(matcher->group(1));
    }
    else
    {
        log_error("Failed to get URL for video with id %s\n",
                    video_id.c_str());
        return nil;
    }

    params = _(YOUTUBE_URL_GET) + params;

    buffer = url->download(curl_handle, params, &retcode, true);

    matcher = redirectLocation->matcher(buffer->toString());
    if (matcher->next())
    {
        return trim_string(matcher->group(1));
    }

    if (retcode != 303)
    {
        log_error("Unexpected YouTube reply: %ld", retcode);
        return nil;
    }

    log_error("Could not retrieve YouTube video URL\n");
    return nil;
}

#endif//YOUTUBE
