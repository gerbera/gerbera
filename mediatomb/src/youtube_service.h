/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_service.h - this file is part of MediaTomb.
    
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

/// \file youtube_service.h
/// \brief Definition of the YouTubeService class.

#if defined(YOUTUBE) // make sure to add more ifdefs when we get more services

#ifndef __YOUTUBE_SERVICE_H__
#define __YOUTUBE_SERVICE_H__

#include "zmm/zmm.h"

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class YouTubeService : public OnlineService
{
public:
    YouTubeService();
    ~YouTubeService();
    /// \brief Retrieves user specified content from the service and adds
    /// the items to the database.
    virtual void refreshServiceData();
protected:
    // the handle *must never be used from multiple threads*
    CURL *curl_handle;
    // safeguard to ensure the above
    pid_t pid;

    /// \brief This function will retrieve the XML according to the parametrs
    zmm::Ref<mxml::Element> getData(zmm::Ref<Dictionary> params);

    /// \brief This function is installed as a callback for libcurl, when
    /// we download data from a remote site. 
    static size_t dl(void *buf, size_t size, size_t nmemb, void *data);
};

#endif//__ONLINE_SERVICE_H__

#endif//YOUTUBE
