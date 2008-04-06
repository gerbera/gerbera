/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_service.h - this file is part of MediaTomb.
    
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

typedef enum 
{
    YT_request_none = 0,
    YT_request_videos,
    YT_request_stdfeed,
    YT_request_user_favorites,
    YT_request_user_playlists,
    YT_request_user_subscriptions,
    YT_request_popular,
} yt_requests_t;

typedef enum
{
    YT_cat_none = 0,
    YT_cat_film,
    YT_cat_autos,
    YT_cat_animals,
    YT_cat_sports,
    YT_cat_travel,
    YT_cat_short_movies,
    YT_cat_videoblog,
    YT_cat_gadgets,
    YT_cat_comedy,
    YT_cat_people,
    YT_cat_news,
    YT_cat_entertainment,
    YT_cat_education,
    YT_cat_howto,
    YT_cat_nonprofit,
    YT_cat_tech,
} yt_categories_t;

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
    virtual zmm::Ref<zmm::Object> defineServiceTask(zmm::Ref<mxml::Element> xmlopt);

    /// \brief Get the human readable name of a particular request type, i.e.
    /// did we request Favorites or Featured videos, etc.
    static zmm::String getRequestName(yt_requests_t request);

    /// \brief Get the human readable category name
    static zmm::String getCategoryName(yt_categories_t category);

protected:
    // the handle *must never be used from multiple threads*
    CURL *curl_handle;
    // safeguard to ensure the above
    pthread_t pid;

    // url retriever class
    zmm::Ref<URL> url;

    /// \brief This function will retrieve the XML according to the parametrs
    zmm::Ref<mxml::Element> getData(zmm::String url_part, zmm::Ref<Dictionary> params);

    /// \brief This class defines the so called "YouTube task", the task
    /// holds various parameters that are needed to perform. A task means
    /// the process of downloading, parsing service data and creating
    /// CdsObjects.
    class YouTubeTask : public zmm::Object
    {
    public:
        YouTubeTask();

        /// \brief Method identifier
        yt_requests_t request;

        /// \brief Category identifier, only used with the
        /// YT_list_by_category_and_tag method
        yt_categories_t category;

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

        /// \brief Current page for requests that require paging
        int current_page;

        /// \brief Playlist name defined by the user (only set when retrieving
        /// playlists)
        zmm::String playlist_name;
    };

    /// \brief task that we will be working with when refreshServiceData is
    /// called.
    int current_task;

    /// \brief utility function, returns true if method supports paging
    bool hasPaging(yt_requests_t request);

    // helper functions for parsing config.xml
    zmm::String getCheckAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    int getCheckPosIntAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    void addPagingParams(zmm::Ref<mxml::Element> xml, 
                         zmm::Ref<YouTubeTask> task);
    zmm::String getRegion(zmm::Ref<mxml::Element> xml);
    zmm::String getFeed(zmm::Ref<mxml::Element> xml);
};

#endif//__ONLINE_SERVICE_H__

#endif//YOUTUBE
