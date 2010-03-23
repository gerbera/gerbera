/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_service.h - this file is part of MediaTomb.
    
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

/// \file youtube_service.h
/// \brief Definition of the YouTubeService class.

#if defined(YOUTUBE) // make sure to add more ifdefs when we get more services

#ifndef __YOUTUBE_SERVICE_H__
#define __YOUTUBE_SERVICE_H__

#include "zmm/zmm.h"
#include "mxml/mxml.h"
#include "online_service.h"
#include "url.h"
#include "dictionary.h"
#include "youtube_content_handler.h"
#include <curl/curl.h>

typedef enum 
{
    YT_request_none = 0,
    YT_request_video_search,
    YT_request_stdfeed,
    YT_request_user_favorites,
    YT_request_user_playlists,
    YT_request_user_subscriptions,
    YT_request_user_uploads,
    YT_subrequest_playlists,
    YT_subrequest_subscriptions,
} yt_requests_t;

typedef enum
{
    YT_region_au = 0,
    YT_region_br,
    YT_region_ca,
    YT_region_fr,
    YT_region_de,
    YT_region_gb,
    YT_region_nl,
    YT_region_hk,
    YT_region_ie,
    YT_region_it,
    YT_region_jp,
    YT_region_mx,
    YT_region_nz,
    YT_region_pl,
    YT_region_ru,
    YT_region_kr,
    YT_region_es,
    YT_region_tw,
    YT_region_us,
    YT_region_none,
} yt_regions_t;

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class YouTubeService : public OnlineService
{
public:
    YouTubeService();
    ~YouTubeService();
    /// \brief Retrieves user specified content from the service and adds
    /// the items to the database.
    virtual bool refreshServiceData(zmm::Ref<Layout> layout);

    /// \brief Get the type of the service (i.e. YouTube, Shoutcast, etc.)
    virtual service_type_t getServiceType();

    /// \brief Get the human readable name for the service
    virtual zmm::String getServiceName();

    /// \brief Parse the xml fragment from the configuration and gather
    /// the user settings in a service task structure.
    virtual zmm::Ref<zmm::Object> defineServiceTask(zmm::Ref<mxml::Element> xmlopt, zmm::Ref<zmm::Object> params);

    /// \brief Get the human readable name of a particular request type, i.e.
    /// did we request Favorites or Featured videos, etc.
    static zmm::String getRequestName(yt_requests_t request);

    /// \brief Get the human readable category name
    static zmm::String getRegionName(yt_regions_t region_code);

protected:
    // the handle *must never be used from multiple threads*
    CURL *curl_handle;
    // safeguard to ensure the above
    pthread_t pid;

    // url retriever class
    zmm::Ref<URL> url;

    /// \brief This function will retrieve the XML according to the parametrs
    zmm::Ref<mxml::Element> getData(zmm::String url_part, zmm::Ref<Dictionary> params, bool construct_url = true);

    /// \brief This class defines the so called "YouTube task", the task
    /// holds various parameters that are needed to perform. A task means
    /// the process of downloading, parsing service data and creating
    /// CdsObjects.
    class YouTubeTask : public zmm::Object
    {
    public:
        YouTubeTask();

        /// \brief Request identifier
        yt_requests_t request;

        /// \brief Region setting 
        yt_regions_t region;

        /// \brief Constructed URL that will be prepended with the base and
        /// appended with parameters.
        zmm::String url_part;
        
        /// \brief It was so nice when using with the REST API, now we will 
        /// have to convert the parameters to a specific string.
        zmm::Ref<Dictionary> parameters;
        
        /// \brief Amount of items that we are allowed to get.
        int amount;

        /// \brief Amount of items that have been fetched.
        int amount_fetched;

        /// \brief Starting index of the item to fetch
        int start_index;

        /// \brief Starging index as specified in the configuration by the user
        int cfg_start_index;

        /// \brief Name of the actual subrequest
        zmm::String sub_request_name;

        /// \brief Special requests have a subfeed
        zmm::Ref<YouTubeSubFeed> subfeed;
        int subfeed_index;

        /// \brief Task must be removed from the tasklist after one time
        /// execution
        bool kill;
    };

    /// \brief task that we will be working with when refreshServiceData is
    /// called.
    int current_task;


    // helper functions for parsing config.xml
//    zmm::String getCheckAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
//    int getCheckPosIntAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    void getPagingParams(zmm::Ref<mxml::Element> xml, 
                         zmm::Ref<YouTubeTask> task);
    void addTimeParams(zmm::Ref<mxml::Element> xml, zmm::Ref<YouTubeTask> task);
    yt_regions_t getRegion(zmm::Ref<mxml::Element> xml);
    zmm::String getFeed(zmm::Ref<mxml::Element> xml);

    // subrequests are spawned as one time tasks, they are removed from the 
    // task list after one time execution - this function takes care of it
    void killOneTimeTasks(zmm::Ref<zmm::Array<zmm::Object> > tasklist);
};

#endif//__ONLINE_SERVICE_H__

#endif//YOUTUBE
