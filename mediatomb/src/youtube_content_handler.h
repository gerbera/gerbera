/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_content_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef YOUTUBE

#ifndef __YOUTUBE_DATA_HANDLER_H__
#define __YOUTUBE_DATA_HANDLER_H__

#define YOUTUBE_SERVICE                     "YouTube"
#define YOUTUBE_SERVICE_ID                  "Y"

#define YOUTUBE_AUXDATA_KEYWORDS            "kwd"
#define YOUTUBE_AUXDATA_AVG_RATING          "avr"
#define YOUTUBE_AUXDATA_AUTHOR              "aut"
#define YOUTUBE_AUXDATA_FAVORITE_COUNT      "fcnt"
#define YOUTUBE_AUXDATA_VIEW_COUNT          "vcnt"
#define YOUTUBE_AUXDATA_RATING_COUNT        "rcount"
#define YOUTUBE_AUXDATA_FEED                "feed"
#define YOUTUBE_AUXDATA_MAIN_REQUEST_NAME   "mreq"
#define YOUTUBE_AUXDATA_REQUEST             "req"
#define YOUTUBE_AUXDATA_CATEGORY            "cat"


#include "zmmf/zmmf.h"
#include "mxml/mxml.h"
#include "cds_objects.h"

/// \brief This class holds the subfeed data for special requests
class YouTubeSubFeed : public zmm::Object
{
public:
    YouTubeSubFeed();
    /// \brief Some requests return a list of items which point to 
    /// further requests (i.e. subscriptions point to an XML which 
    /// only contains the links which have to be used in subsequent 
    /// requests before the actual video data can be retrieved
    zmm::Ref<zmm::Array<zmm::StringBase> > links;
    zmm::String title;
};

/// \brief this class is responsible for creating objects from the YouTube
/// metadata XML.
class YouTubeContentHandler : public zmm::Object
{
public:
    /// \brief Sets the service XML from which we will extract the objects.
    /// \return false if service XML contained an error status.
    bool setServiceContent(zmm::Ref<mxml::Element> service);

    /// \brief retrieves an object from the service.
    ///
    /// Each invokation of this funtion will return a new object,
    /// when the whole service XML is parsed and no more objects are left,
    /// this function will return nil.
    ///
    /// \return CdsObject or nil if there are no more objects to parse.
    zmm::Ref<CdsObject> getNextObject();

    /// \brief Retrieves a list of feed URLs from a subscription request along
    /// with the feeds name.
    /// 
    /// Some requests, like subscriptions, do not return the video metadata,
    /// but return an XML which holds URLs to further requests, only these
    /// further requests give us the actual video data. So, we will gather
    /// the request links in an array and then walk the array via the
    /// above setServiceContent/getNextObject methods.
    zmm::Ref<YouTubeSubFeed> getSubFeed(zmm::Ref<mxml::Element> feedxml);

protected:
    zmm::Ref<mxml::Element> service_xml;
    int current_node_index;
    int channel_child_count;
    zmm::String thumb_mimetype;
    zmm::String feed_name;
};

#endif//__YOUTUBE_CONTENT_HANDLER_H__

#endif//YOUTUBE
