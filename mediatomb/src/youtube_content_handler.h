/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_content_handler.h - this file is part of MediaTomb.
    
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

/// \file youtube_content_handler.h
/// \brief Definitions of the YouTubeContentHandler class.

#if defined(YOUTUBE) // make sure to add more ifdefs when we get more services

#ifndef __YOUTUBE_DATA_HANDLER_H__
#define __YOUTUBE_DATA_HANDLER_H__

#include "online_service_content_handler.h"

#define YOUTUBE_SERVICE     "YouTube"
#define YOUTUBE_SERVICE_ID   "yt"
#define YOUTUBE_VIDEO_ID    "vid"

#define YOUTUBE_AUXDATA_TAGS            "tags"
#define YOUTUBE_AUXDATA_AVG_RATING      "rating"
#define YOUTUBE_AUXDATA_AUTHOR          "author"
#define YOUTUBE_AUXDATA_COMMENT_COUNT   "ccount"
#define YOUTUBE_AUXDATA_VIEW_COUNT      "vcount"
#define YOUTUBE_AUXDATA_RATING_COUNT    "rcount"


#include "zmmf/zmmf.h"
#include "mxml/mxml.h"

/// \brief this class is an interface for parsing content data of various 
/// online services.
class YouTubeContentHandler : public OnlineServiceContentHandler
{
public:
    /// \brief Sets the service XML from which we will extract the objects.
    ///
    /// \todo specify rules of what to extarct (i.e. only a specific genre,etc)
    virtual void setServiceContent(zmm::Ref<mxml::Element> service);

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nil.
    ///
    /// \return CdsObject or nil if there are no more objects to parse.
    virtual zmm::Ref<CdsObject> getNextObject();


protected:
    int current_video_node_index;
    int video_list_child_count;
};

#endif//__YOUTUBE_CONTENT_HANDLER_H__

#endif//YOUTUBE
