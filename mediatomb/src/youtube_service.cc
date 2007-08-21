/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_service.cc - this file is part of MediaTomb.
    
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

/// \file youtube_service.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef YOUTUBE // make sure to add more ifdefs when we get more services

#include "zmm/zmm.h"
#include "youtube_service.h"
#include "youtube_content_handler.h"
#include "content_manager.h"

using namespace zmm;
using namespace mxml;

// REST API defines

// Please do not use our deveoper ID elsewhere/in other projects.
// If you need to create your own, you can easily do so at http://youtube.com/
#define MT_DEV_ID                       "jyNuyIwi30E"

// Base request URL
#define REST_BASE_URL                   "http://www.youtube.com/api2_rest?"

// REST API methods
// dev_id=dev_id&user=user
#define REST_METHOD_LIST_FAVORITE       "youtube.users.list_favorite_videos"

// dev_id=dev_id&tag=tag&page=page&per_page=per_page
#define REST_METHOD_LIST_BY_TAG         "youtube.videos.list_by_tag"

// dev_id=dev_id&user=user&page=page&per_page=per_page
#define REST_METHOD_LIST_BY_USER        "youtube.videos.list_by_user"

// dev_id=dev_id
#define REST_METHOD_LIST_FEATURED       "youtube.videos.list_featured"

// dev_id=dev_id&id=id&page=page&per_page=per_page
#define REST_METHOD_LIST_BY_PLAYLIST    "youtube.videos.list_by_playlist"

// dev_id=dev_id&time_range=time_range
#define REST_METHOD_LIST_POPULAR        "youtube.videos.list_popular"

// dev_id=dev_id& category_id=category_id &tag=tag&page=page&per_page=per_page
#define REST_METHOD_LIST_BY_CAT_AND_TAG "youtube.videos.list_by_category_and_tag"
// REST API parameters
#define REST_PARAM_DEV_ID                   "dev_id"
#define REST_PARAM_TAG                      "tag"
#define REST_PARAM_ITEMS_PER_PAGE           "per_page"
#define REST_PARAM_PAGE_NUMBER              "page"
#define REST_PARAM_USER                     "user"
#define REST_PARAM_METHOD                   "method"
#define REST_PARAM_PLAYLIST_ID              "id"
#define REST_PARAM_CATEGORY_ID              "category_id"

// REST API time range values
#define REST_VALUE_TIME_RANGE_ALL           "all"
#define REST_VALUE_TIME_RANGE_DAY           "day"
#define REST_VALUE_TIME_RANGE_WEEK          "week"
#define REST_VALUE_TIME_RANGE_MONTH         "month"

// REST API available categories
#define REST_VALUE_CAT_FILMS_AND_ANIMATION  "1"
#define REST_VALUE_CAT_AUTOS_AND_VEHICLES   "2"
#define REST_VALUE_CAT_COMEDY               "23"
#define REST_VALUE_CAT_ENTERTAINMENT        "24"
#define REST_VALUE_CAT_MUSIC                "10"
#define REST_VALUE_CAT_NEWS_AND_POLITICS    "25"
#define REST_VALUE_CAT_PEOPLE_AND_BLOGS     "22"
#define REST_VALUE_CAT_PETS_AND_ANIMALS     "15"
#define REST_VALUE_CAT_HOW_TO_AND_DIY       "26"
#define REST_VALUE_CAT_SPORTS               "17"
#define REST_VALUE_CAT_TRAVEL_AND_PLACES    "19"
#define REST_VALUE_CAT_GADGETS_AND_GAMES    "20"

// REST API min/max items per page values
#define REST_VALUE_PER_PAGE_MIN             "20"
#define REST_VALUE_PER_PAGE_MAX             "100"

// REST API error codes
#define REST_ERROR_INTERNAL                 "1"
#define REST_ERROR_BAD_XMLRPC               "2"
#define REST_ERROR_UNKOWN_PARAM             "3"
#define REST_ERROR_MISSING_REQURED_PARAM    "4"
#define REST_ERROR_MISSING_METHOD           "5"
#define REST_ERROR_UNKNOWN_METHOD           "6"
#define REST_ERROR_MISSING_DEV_ID           "7"
#define REST_ERROR_BAD_DEV_ID               "8"
#define REST_ERROR_NO_SUCH_USER             "101"

YouTubeService::YouTubeService()
{
   curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    pid = pthread_self();

    url = Ref<GetURL>(new GetURL());
}

YouTubeService::~YouTubeService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

Ref<Element> YouTubeService::getData(Ref<Dictionary> params)
{
    long retcode;
    Ref<StringConverter> sc = StringConverter::i2i();

    // dev id is requied in each call
    params->put(_(REST_PARAM_DEV_ID), _(MT_DEV_ID));
    String URL = _(REST_BASE_URL) + params->encode();

    log_debug("Retrieving URL: %s\n", URL.c_str());
   
    Ref<StringBuffer> buffer;
    try 
    {
        log_debug("DOWNLOADING URL: %s\n", URL.c_str());
        buffer = url->download(curl_handle, URL, &retcode, false, true);
    }
    catch (Exception ex)
    {
        log_error("Failed to download YouTube XML data\n");
        return nil;
    }

    if (buffer == nil)
        return nil;

    if (retcode != 200)
        return nil;

    log_debug("GOT BUFFER\n%s\n", buffer->toString().c_str()); 
    write_text_file(_("/tmp/heli.xml"), buffer->toString()); 
    Ref<Parser> parser(new Parser());
    try
    {
        return parser->parseString(sc->convert(buffer->toString()));
    }
    catch (ParseException pe)
    {
        log_error("Error parsing YouTube XML %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        return nil;
    }
    catch (Exception ex)
    {
        log_error("Error parsing YouTube XML %s\n", ex.getMessage().c_str());
        return nil;
    }
    
    return nil;
}

// obviously the config.xml should provide a way to define what we want,
// for testing purposes we will stat by importing the featured videos
void YouTubeService::refreshServiceData(Ref<Layout> layout)
{
    log_debug("------------------------>> REFRESHING SERVICE << ----------\n");
    // the layout is in full control of the service items
    if (layout == nil)
    {
        log_debug("-- no layout given!!!!!!!!\n");
        return;
    }

    // safeguard to make sure that our curl_handle is not used in multiple
    // threads
    if (pid != pthread_self())
        throw _Exception(_("Not allowed to call refreshServiceData from different threads!"));


    /// \todo define search rules in the configuration and use them here
    Ref<Dictionary> params(new Dictionary());
    params->put(_(REST_PARAM_METHOD), _(REST_METHOD_LIST_BY_TAG));
    params->put(_(REST_PARAM_TAG), _("tank"));
//    params->put(_(REST_PARAM_PAGE_NUMBER), _("1"));
//    params->put(_(REST_PARAM_ITEMS_PER_PAGE), _("10"));

    /// \todo get the total number and make sure we do not stop after the
    /// first 100 items but fetch as much stuff as the user wants

    Ref<Element> reply = getData(params);

    Ref<YouTubeContentHandler> yt(new YouTubeContentHandler());
    if (reply != nil)
        yt->setServiceContent(reply);
    else
    {
        log_debug("Failed to get XML content from YouTube service\n");
        return;
    }

    /// \todo make sure the CdsResourceManager knows whats going on,
    /// since those items do not contain valid links but need to be
    /// processed later on (i.e. need to figure out the real link to the flv)
    Ref<CdsObject> obj;
    do
    {
        obj = yt->getNextObject();
        if (obj == nil)
            break;

        obj->setVirtual(true);
        /// \todo we need a function that would do a lookup on the special
        /// service ID and tell is uf a particular object already exists
        /// in the database
//        Ref<CdsObject> old = Storage::getInstance()->findObjectByPath(obj->getLocation());
        Ref<CdsObject> old;
        if (old == nil)
        {
            log_debug("Found new object!!!!\n");
            layout->processCdsObject(obj);
        }
        else
        {
            log_debug("NEED TO UPDATE EXISTING OBJECT WITH ID: %s\n",
                    obj->getLocation());
        }
    }
    while (obj != nil);
}

#endif//YOUTUBE
