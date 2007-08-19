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

#include <curl/curl.h>
#include "zmm/zmm.h"
#include "youtube_service.h"

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
#define REST_PARAM_DEV_ID               "dev_id"
#define REST_PARAM_TAG                  "tag"
#define REST_PARAM_ITEMS_PER_PAGE       "per_page"
#define REST_PARAM_PAGE_NUMBER          "page"
#define REST_PARAM_USER                 "user"
#define REST_PARAM_METHOD               "method"
#define REST_PARAM_PLAYLIST_ID          "id"
#define REST_PARAM_CATEGORY_ID          "category_id"

// REST API time range values
#define REST_VALUE_TIME_RANGE_ALL       "all"
#define REST_VALUE_TIME_RANGE_DAY       "day"
#define REST_VALUE_TIME_RANGE_WEEK      "week"
#define REST_VALUE_TIME_RANGE_MONTH     "month"

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
#define REST_VALUE_PER_PAGE_MIN         "20"
#define REST_VALUE_PER_PAGE_MAX         "100"

// REST API error codes
#define REST_ERROR_INTERNAL             "1"
#define REST_ERROR_BAD_XMLRPC           "2"
#define REST_ERROR_UNKOWN_PARAM         "3"
#define REST_ERROR_MISSING_REQURED_PARAM "4"
#define REST_ERROR_MISSING_METHOD       "5"
#define REST_ERROR_UNKNOWN_METHOD       "6"
#define REST_ERROR_MISSING_DEV_ID       "7"
#define REST_ERROR_BAD_DEV_ID           "8"
#define REST_ERROR_NO_SUCH_USER         "101"

YouTubeService::YouTubeService()
{
   curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    pid = pthread_self();
}

YouTubeService::~YouTubeService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

Ref<Element> YouTubeService::getData(Ref<Dictionary> params)
{
    CURLcode res;
    long retcode;
    char error_buffer[CURL_ERROR_SIZE] = {'\0'};

    if (pid != pthread_self())
        throw _Exception(_("Not allowed to call getVideoURL from different threads!"));
    
    // dev id is requied in each call
    params->put(_(REST_PARAM_DEV_ID), _(MT_DEV_ID));
    String URL = _(REST_BASE_URL) + params->encode();

    curl_easy_reset(curl_handle);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl_handle, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, YouTubeVideoURL::dl);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)buffer.getPtr());
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, error_buffer);

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &retcode);
    if (res != CURLE_OK)
    {
        log_error("%s\n", error_buffer);
        throw _Exception(String(error_buffer));
    }

    if (retcode != 200)
    {
        log_warning("Failed to get URL for video with id %s,http response %d\n",
                video_id.c_str(), retcode);
        return nil;
    }
    
    Ref<Parser> parser(new Parser());
    try
    {
        return parser->parseString(buffer->toString());
    }
    catch (ParseException pe)
    {
        log_error("Error parsing YouTube XML %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        return nil;
    }
    catch (Exceptin ex)
    {
        log_error("Error parsing YouTube XML %s\n", ex.getMessage().c_str());
        return nil;
    }
    
    return nil;
}

size_t YouTubeVideoURL::dl(void *buf, size_t size, size_t nmemb, void *data)
{   
    StringBuffer *buffer = (StringBuffer *)data;
    if (buffer == NULL)
        return 0;
    
    size_t s = size * nmemb;
    *buffer << String((char *)buf, s);

    return s;
}

// obviously the config.xml should provide a way to define what we want,
// for testing purposes we will stat by importing the featured videos
void YouTubeService::refreshServiceData()
{
    if (pid != pthread_self())
        throw _Exception(_("Not allowed to call refreshServiceData from different threads!"));

}

#endif//YOUTUBE
