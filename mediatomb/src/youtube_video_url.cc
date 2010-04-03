/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_video_url.cc - this file is part of MediaTomb.
    
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

/// \file youtube_video_url.cc
/// \brief Definitions of the Transcoding classes. 

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef YOUTUBE

#include <pthread.h>

#include "youtube_video_url.h"
#include "tools.h"
#include "url.h"

using namespace zmm;

#define YOUTUBE_URL_PARAMS_REGEXP   "var swfHTML.*\\;"
#define YOUTUBE_URL_LOCATION_REGEXP "\nLocation: (http://[^\n]+)\n"
#define YOUTUBE_URL_WATCH           "http://www.youtube.com/watch?v="
#define YOUTUBE_URL_GET             "http://www.youtube.com/get_video?" 
#define YOUTUBE_URL_PARAM_VIDEO_ID  "video_id"
#define YOUTUBE_URL_PARAM_T_REGEXP  ".*\&t=([^\&]+)\&"
#define YOUTUBE_URL_PARAM_T         "t"
#define YOUTUBE_IS_HD_AVAILABLE_REGEXP  "IS_HD_AVAILABLE[^:]*: *([^,]*)"
YouTubeVideoURL::YouTubeVideoURL()
{
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    reVideoURLParams = Ref<RExp>(new RExp());
    reVideoURLParams->compile(_(YOUTUBE_URL_PARAMS_REGEXP));
    redirectLocation = Ref<RExp>(new RExp());
    redirectLocation->compile(_(YOUTUBE_URL_LOCATION_REGEXP));
    param_t = Ref<RExp>(new RExp());
    param_t->compile(_(YOUTUBE_URL_PARAM_T_REGEXP));

    HD = Ref<RExp>(new RExp());
    HD->compile(_(YOUTUBE_IS_HD_AVAILABLE_REGEXP));

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

String YouTubeVideoURL::getVideoURL(String video_id, bool mp4, bool hd)
{
    long retcode; 
    String flv_location;
    String watch;
#ifdef TOMBDEBUG
    bool verbose = true;
#else
    bool verbose = false;
#endif

   /*
// ###########################################################

    String swfargs = read_text_file("/home/jin/Work/UPnP/MediaTomb/YouTube/swf_args_new2.txt");
    
    Ref<Matcher> m2 = param_t->matcher(swfargs);
    
    if (m2->next())
    {
        String hmm = m2->group(1);
        if (string_ok(hmm))
            log_debug("############### t: %s\n", hmm.c_str());
        else 
            log_debug("no match?\n");
    }
    throw _Exception(_("OVER"));
*/

// ###########################################################


    if (!string_ok(video_id))
        throw _Exception(_("No video ID specified!"));

    watch = _(YOUTUBE_URL_WATCH) + video_id;

    Ref<URL> url(new URL(YOUTUBE_PAGESIZE));

    Ref<StringBuffer> buffer = url->download(watch, &retcode, curl_handle,
                                             false, verbose, true);
    if (retcode != 200)
    {
        throw _Exception(_("Failed to get URL for video with id ")
                         + watch + _("HTTP response code: ") + 
                         String::from(retcode));
    }

    log_debug("------> GOT BUFFER %s\n", buffer->toString().c_str());

    Ref<Matcher> matcher =  reVideoURLParams->matcher(buffer->toString());
    String params;
    if (matcher->next())
    {
//        params = trim_string(matcher->group(1));
        params = trim_string( matcher->group( 0 ) );
      /*
        int brace = params.index( '{' );
        if ( brace > 0 )
            params = params.substring( brace );
        brace = params.index( '}' );
        if ( brace > 0 )
            params = params.substring( 0, brace + 1 );
            */
        Ref<Matcher> m2 = param_t->matcher(params);
        if (m2->next())
        {
            String hmm = m2->group(1);
            if (string_ok(hmm))
                params = hmm; 
            else 
            {
                throw _Exception(_("Could not retrieve \"t\" parameter."));
            }
        }
    }
    else
    {
        throw _Exception(_("Failed to get URL for video with id (step 1)") + video_id);
    }

    params = _(YOUTUBE_URL_GET) + YOUTUBE_URL_PARAM_VIDEO_ID + '=' + 
             video_id + '&' + YOUTUBE_URL_PARAM_T + '=' + params;

    if (mp4)
    {
        String format = _("&fmt=18");
        
        if (hd)
        {
            matcher = HD->matcher(buffer->toString());
            if (matcher->next())
            {
                if (trim_string(matcher->group(1)) == "true")
                    format = _("&fmt=22");
            }
        }
                    
        params = params + format;
    }

    buffer = url->download(params, &retcode, curl_handle, true, verbose, true);

    matcher = redirectLocation->matcher(buffer->toString());
    if (matcher->next())
    {
        if (string_ok(trim_string(matcher->group(1))))
            return trim_string(matcher->group(1));
        else
            throw _Exception(_("Failed to get URL for video with id (step 2)")+ 
                             video_id);
    }

    if (retcode != 303)
    {
        throw _Exception(_("Unexpected reply from YouTube: ") + 
                         String::from(retcode));
    }

    throw _Exception(_("Could not retrieve YouTube video URL"));
}

#endif//YOUTUBE
