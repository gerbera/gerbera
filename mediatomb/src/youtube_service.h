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
#include "mxml/mxml.h"
#include "online_service.h"
#include "url.h"
#include "dictionary.h"

typedef enum 
{
    YT_list_none = 0,
    YT_list_favorite,
    YT_list_by_tag,
    YT_list_by_user,
    YT_list_featured,
    YT_list_by_playlist,
    YT_list_popular,
    YT_list_by_category_and_tag
} yt_methods_t;

typedef enum
{
    YT_cat_none                     = 0,
    YT_cat_film_and_animation       = 1,
    YT_cat_autos_and_vehicles       = 2,
    YT_cat_music                    = 10,
    YT_cat_pets_and_animals         = 15,
    YT_cat_sports                   = 17,
    YT_cat_travel_and_places        = 19,
    YT_cat_gadgets_and_games        = 20,
    YT_cat_people_and_blogs         = 22,
    YT_cat_comedy                   = 23,
    YT_cat_entertainment            = 24,
    YT_cat_news_and_politics        = 25,
    YT_cat_howto_and_diy            = 26
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
    static zmm::String getRequestName(yt_methods_t method);

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
    zmm::Ref<mxml::Element> getData(zmm::Ref<Dictionary> params);

    /// \brief This class defines the so called "YouTube task", the task
    /// holds various parameters that are needed to perform. A task means
    /// the process of downloading, parsing service data and creating
    /// CdsObjects.
    class YouTubeTask : public zmm::Object
    {
    public:
        YouTubeTask();

        /// \brief Method identifier
        yt_methods_t method;

        /// \brief Category identifier, only used with the
        /// YT_list_by_category_and_tag method
        yt_categories_t category;
        
        /// \brief parameter=value for the REST API URL
        zmm::Ref<Dictionary> parameters;
        
        /// \brief Amount of items that we are allowed to get.
        int amount;

        /// \brief Amount of items that have been fetched.
        int amount_fetched;

        /// \brief Current page for requests that require paging
        int current_page;

        /// \brief Playlist name defined by the user (only set if method
        /// equals YT_list_by_playlist)
        zmm::String playlist_name;
    };

    /// \brief task that we will be working with when refreshServiceData is
    /// called.
    int current_task;

    /// \brief utility function, returns true if method supports paging
    bool hasPaging(yt_methods_t method);

    // helper functions for parsing config.xml
    zmm::String getCheckAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    int getCheckPosIntAttr(zmm::Ref<mxml::Element> xml, zmm::String attrname);
    void addPagingParams(zmm::Ref<mxml::Element> xml, 
                         zmm::Ref<YouTubeTask> task);
};

#endif//__ONLINE_SERVICE_H__

#endif//YOUTUBE
