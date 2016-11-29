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
#define YOUTUBE_URL_PARAM_T_REGEXP  ".*&t=([^&]+)&"
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

    /*
     * NOTE ON PATCH:
     *
     * The original code does not seem to work anymore.
     *
     * I have commented-out all the original code, and instead
     * replaced it with a call/exec to youtube-dl (this is a separate/stand-alone python script).
     *
     * Available at http://rg3.github.io/youtube-dl/
     *
     *
     * The current code works on a/my samsung TV. I have not tested it further on other devices.
     * (I needed a quick fix, because I wanted to watch some video's.  :) )
     *
     * I thought I would share the results.
     *
     * Suggestions / feedback  -> bas-patch@tcfaa.nl
     *
     * Regards, Bas Nedermeijer
     */ 

    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0)
    {
        // close reading end in the child
        close(pipefd[0]);

        // send stdout to the pipe
        dup2(pipefd[1], 1); 
        // send stderr to the pipe
        dup2(pipefd[1], 2); 

        // this descriptor is no longer needed
        close(pipefd[1]); 

        // This code assumes youtube-dl is available for usage.
        execl("/usr/bin/youtube-dl", "/usr/bin/youtube-dl","-g",watch.c_str(),NULL);
    }
    else
    {
        // parent
        char buffery[8192];
        memset(&buffery[0], 0, sizeof(buffery));

        close(pipefd[1]);  // close the write end of the pipe in the parent

        // Hopefully the read is never called twice, otherwise the buffer will become corrupt.
        while (read(pipefd[0], buffery, sizeof(buffery)) != 0)
        {
        }

       log_debug("------> GOT BUFFER %s\n", buffery);
       String result = _(buffery);

       result = trim_string(result);

       log_debug("------> GOT BUFFER (after trimming) %s\n", result.c_str());

       return result;
    }
}

#endif//YOUTUBE
