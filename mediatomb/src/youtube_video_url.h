/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_video_url.h - this file is part of MediaTomb.
    
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
    
    $Id $
*/

/// \file youtube_video_url.h
/// \brief Definitions of the Transcoding classes. 

#ifdef YOUTUBE

#ifndef __YOUTUBE_VIDEO_URL_H__
#define __YOUTUBE_VIDEO_URL_H__

#include <curl/curl.h>
#include "zmmf/zmmf.h"
#include "zmm/zmm.h"
#include "rexp.h"


/// \brief this class keeps all data associated with one transcoding profile.
class YouTubeVideoURL : public zmm::Object
{
public:
    YouTubeVideoURL();
    ~YouTubeVideoURL();
    
    /// \brief Takes the usual YouTube style URL as argument 
    /// and returns the URL to the associated video.
    ///
    /// \param video_id id of the video
    /// \return the url to the .flv file 
    zmm::String getVideoURL(zmm::String video_id);

protected:
    // the handle *must never be used from multiple threads*
    CURL *curl_handle;
    zmm::Ref<RExp> reVideoURLParams;
    zmm::Ref<RExp> redirectLocation;

    /// \brief This function is installed as a callback for libcurl, when
    /// we download data from a remote site.
    static size_t dl(void *buf, size_t size, size_t nmemb, void *data);
};

#endif//__YOUTUBE_VIDEO_URL_H__

#endif//YOUTUBE
