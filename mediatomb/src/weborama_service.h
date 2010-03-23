/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    weborama_service.h - this file is part of MediaTomb.
    
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

/// \file weborama_service.h
/// \brief Definition of the WeboramaService class.

#ifdef WEBORAMA

#ifndef __WEBORAMA_SERVICE_H__
#define __WEBORAMA_SERVICE_H__

#include "zmm/zmm.h"
#include "mxml/mxml.h"
#include "online_service.h"
#include "url.h"
#include "dictionary.h"
#include <curl/curl.h>


typedef enum
{
    WR_mood_calm_positive = 0,
    WR_mood_positive,
    WR_mood_energetic_positive,
    WR_mood_calm,
    WR_mood_neutral,
    WR_mood_energetic,
    WR_mood_calm_dark,
    WR_mood_dark,
    WR_mood_energetic_dark,
    WR_mood_none,
} wr_mood_t;

typedef enum
{
    WR_genre_electronic = 0,
    WR_genre_alternative,
    WR_genre_jazz,
    WR_genre_metal,
    WR_genre_pop,
    WR_genre_punk,
    WR_genre_rhytm_and_blues,
    WR_genre_rap,
    WR_genre_reggae,
    WR_genre_rock,
    WR_genre_classic,
    WR_genre_soundtrack,
    WR_genre_retro,
    WR_genre_russian_rock,
    WR_genre_russian_pop,
    WR_genre_chanson,
    WR_genre_folk,
    WR_genre_none,
} wr_genre_t;

/// \brief This is an interface for all online services, the function
/// handles adding/refreshing content in the database.
class WeboramaService : public OnlineService
{
public:
    WeboramaService();
    ~WeboramaService();
    /// \brief Retrieves user specified content from the service and adds
    /// the items to the database.
    virtual bool refreshServiceData(zmm::Ref<Layout> layout);

    /// \brief Get the type of the service (i.e. Weborama, Shoutcast, etc.)
    virtual service_type_t getServiceType();

    /// \brief Get the human readable name for the service
    virtual zmm::String getServiceName();

    /// \brief Parse the xml fragment from the configuration and gather
    /// the user settings in a service task structure.
    virtual zmm::Ref<zmm::Object> defineServiceTask(zmm::Ref<mxml::Element> xmlopt, zmm::Ref<zmm::Object> params);

protected:
    // the handle *must never be used from multiple threads*
    CURL *curl_handle;
    // safeguard to ensure the above
    pthread_t pid;

    // url retriever class
    zmm::Ref<URL> url;

    /// \brief This function will retrieve the XML according to the parametrs
    zmm::Ref<mxml::Element> getData(zmm::Ref<Dictionary> params);

    /// \brief task that we will be working with when refreshServiceData is
    /// called.
    int current_task;

    class WeboramaTask : public zmm::Object
    {
    public:
        WeboramaTask();

        /// \brief Name of the task as defined by the user.
        zmm::String name;

        /// \brief Weborama API URL parameters
        zmm::Ref<Dictionary> parameters;

        /// \brief Amount of items that we are allowed to get.
        int amount;

        /// \brief Amount of items that have been fetched.
        int amount_fetched;

        /// \brief Starting index of the item to fetch
        int start_index;
    };

    wr_mood_t getMood(zmm::Ref<mxml::Element> xml);
    wr_genre_t getGenre(zmm::String genre);
    int getGenreID(wr_genre_t genre);
};

#endif//__ONLINE_SERVICE_H__

#endif//WEBORAMA
