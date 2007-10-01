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

#ifdef YOUTUBE 

#include "zmm/zmm.h"
#include "youtube_service.h"
#include "youtube_content_handler.h"
#include "content_manager.h"
#include "string_converter.h"
#include "config_manager.h"
#include "server.h"
#include "storage.h"
#include "tools.h"

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
#define REST_PARAM_TIME_RANGE               "time_range"

// REST API time range values
#define REST_VALUE_TIME_RANGE_ALL           "all"
#define REST_VALUE_TIME_RANGE_DAY           "day"
#define REST_VALUE_TIME_RANGE_WEEK          "week"
#define REST_VALUE_TIME_RANGE_MONTH         "month"

// REST API available categories
// categories are now handled in the enum
/*
#define REST_VALUE_CAT_FILM_AND_ANIMATION   "1"
#define REST_VALUE_CAT_AUTOS_AND_VEHICLES   "2"
#define REST_VALUE_CAT_COMEDY               "23"
#define REST_VALUE_CAT_ENTERTAINMENT        "24"
#define REST_VALUE_CAT_MUSIC                "10"
#define REST_VALUE_CAT_NEWS_AND_POLITICS    "25"
#define REST_VALUE_CAT_PEOPLE_AND_BLOGS     "22"
#define REST_VALUE_CAT_PETS_AND_ANIMALS     "15"
#define REST_VALUE_CAT_HOWTO_AND_DIY        "26"
#define REST_VALUE_CAT_SPORTS               "17"
#define REST_VALUE_CAT_TRAVEL_AND_PLACES    "19"
#define REST_VALUE_CAT_GADGETS_AND_GAMES    "20"
*/

// REST API min/max items per page values
#define REST_VALUE_PER_PAGE_MIN             "1" // allthouth the spec says 20,
                                                // lower values also seem to 
                                                // work
#define REST_VALUE_PER_PAGE_MAX             "100"

#define AMOUNT_ALL                          (-333)
#define PER_PAGE_MIN                        1
#define PER_PAGE_MAX                        100

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

#define CAT_NAME_FILM_AND_ANIM              "Film & Animation"
#define CAT_NAME_AUTOS_AND_VEHICLES         "Autos & Vehicles"
#define CAT_NAME_COMEDY                     "Comedy"
#define CAT_NAME_ENTERTAINMENT              "Entertainment"
#define CAT_NAME_MUSIC                      "Music"
#define CAT_NAME_NEWS_AND_POLITICS          "News & Politics"
#define CAT_NAME_PEOPLE_AND_BLOGS           "People & Blogs"
#define CAT_NAME_PETS_AND_ANIMALS           "Pets & Animals"
#define CAT_NAME_HOWTO_AND_DIY              "Howto & DIY" 
#define CAT_NAME_SPORTS                     "Sports"
#define CAT_NAME_TRAVEL_AND_PLACES          "Travel & Places"
#define CAT_NAME_GADGETS_AND_GAMES          "Gadgets & Games"

#define REQ_NAME_FAVORITES                  "Favorites"
#define REQ_NAME_FEATURED                   "Featured"
#define REQ_NAME_POPULAR                    "Popular"
#define REQ_NAME_PLAYLIST                   "Playlists"
#define REQ_NAME_CATEGORY_AND_TAG           "Categories"

// config.xml defines
#define CFG_CAT_STRING_FILM_AND_ANIM       "films_and_animation"
#define CFG_CAT_STRING_AUTOS_AND_VEHICLES   "autos_and_vehicles"
#define CFG_CAT_STRING_COMEDY               "comedy"
#define CFG_CAT_STRING_ENTERTAINMENT        "entertainment"
#define CFG_CAT_STRING_MUSIC                "music"
#define CFG_CAT_STRING_NEWS_AND_POLITICS    "news_and_politics"
#define CFG_CAT_STRING_PEOPLE_AND_BLOGS     "people_and_blogs"
#define CFG_CAT_STRING_PETS_AND_ANIMALS     "pets_and_animals"
#define CFG_CAT_STRING_HOWTO_AND_DIY        "howto_and_diy"
#define CFG_CAT_STRING_SPORTS               "sports"
#define CFG_CAT_STRING_TRAVEL_AND_PLACES    "travel_and_places"
#define CFG_CAT_STRING_GADGETS_AND_GAMES    "gadgets_and_games"

#define CFG_METHOD_FAVORITES                "favorites" 
#define CFG_METHOD_TAG                      "tag"
#define CFG_METHOD_USER                     "user"
#define CFG_METHOD_FEATURED                 "featured"
#define CFG_METHOD_PLAYLIST                 "playlist"
#define CFG_METHOD_POPULAR                  "popular"
#define CFG_METHOD_CATEGORY_AND_TAG         "category-and-tag"

#define CFG_OPTION_USER                     "user"
#define CFG_OPTION_TAG                      "tag"
#define CFG_OPTION_STARTPAGE                "start-page"
#define CFG_OPTION_AMOUNT                   "amount"
#define CFG_OPTION_PLAYLIST_ID              "id"
#define CFG_OPTION_TIME_RANGE               "time-range"
#define CFG_OPTION_CATEGORY                 "category"

YouTubeService::YouTubeService()
{
    url = Ref<URL>(new URL());
    pid = 0;
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    current_task = 0;
}

YouTubeService::~YouTubeService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

YouTubeService::YouTubeTask::YouTubeTask()
{
    parameters = zmm::Ref<Dictionary>(new Dictionary());
    parameters->put(_(REST_PARAM_DEV_ID), _(MT_DEV_ID));
    method = YT_list_none;
    category = YT_cat_none;
    amount = 0;
    amount_fetched = 0;
    current_page = 0;
}


service_type_t YouTubeService::getServiceType()
{
    return OS_YouTube;
}

String YouTubeService::getServiceName()
{
    return _("YouTube");
}

char YouTubeService::getStoragePrefix()
{
    return OnlineService::getStoragePrefix(OS_YouTube);
}


String YouTubeService::getRequestName(yt_methods_t method)
{
    String temp;

    switch (method)
    {
        case YT_list_favorite:
            temp = _(REQ_NAME_FAVORITES);
            break;
        case YT_list_featured:
            temp = _(REQ_NAME_FEATURED);
            break;
        case YT_list_popular:
            temp = _(REQ_NAME_POPULAR);
            break;
        case YT_list_by_playlist:
            temp = _(REQ_NAME_PLAYLIST);
            break;
        case YT_list_by_category_and_tag:
            temp = _(REQ_NAME_CATEGORY_AND_TAG);
            break;
        case YT_list_none:
        case YT_list_by_tag:
        case YT_list_by_user:
        default:
            temp = nil;
            break;
    }

    return temp;
}

String YouTubeService::getCategoryName(yt_categories_t category)
{
    String temp;

    switch (category)
    {
        case YT_cat_film_and_animation:
            temp = _(CAT_NAME_FILM_AND_ANIM);
            break;
        case YT_cat_autos_and_vehicles:
            temp = _(CAT_NAME_AUTOS_AND_VEHICLES);
            break;
        case YT_cat_music:
            temp = _(CAT_NAME_MUSIC);
            break;
        case YT_cat_pets_and_animals:
            temp = _(CAT_NAME_PETS_AND_ANIMALS);
            break;
        case YT_cat_sports:
            temp = _(CAT_NAME_SPORTS);
            break;
        case YT_cat_travel_and_places:
            temp = _(CAT_NAME_TRAVEL_AND_PLACES);
            break;
        case YT_cat_gadgets_and_games:
            temp = _(CAT_NAME_GADGETS_AND_GAMES);
            break;
        case YT_cat_people_and_blogs:
            temp = _(CAT_NAME_PEOPLE_AND_BLOGS);
            break;
        case YT_cat_comedy:
            temp = _(CAT_NAME_COMEDY);
            break;
        case YT_cat_entertainment:
            temp = _(CAT_NAME_ENTERTAINMENT);
            break;
        case YT_cat_news_and_politics:
            temp = _(CAT_NAME_NEWS_AND_POLITICS);
            break;
        case YT_cat_howto_and_diy:
            temp = _(CAT_NAME_HOWTO_AND_DIY);
            break;
        case YT_cat_none:
        default:
            temp = nil;
            break;
    }

    return temp;
}


String YouTubeService::getCheckAttr(Ref<Element> xml, String attrname)
{
    String temp = xml->getAttribute(attrname);
    if (string_ok(temp))
        return temp;
    else
        throw _Exception(_("Tag <") + xml->getName() +
                         _("> is missing the requred \"") + attrname + 
                         _("\" attribute!"));
    return nil;
}

int YouTubeService::getCheckPosIntAttr(Ref<Element> xml, String attrname)
{
    int itmp;
    String temp = xml->getAttribute(attrname);
    if (string_ok(temp))
        itmp = temp.toInt();
    else
        throw _Exception(_("Tag <") + xml->getName() +
                         _("> is missing the requred \"") + attrname + 
                         _("\" attribute!"));

    if (itmp < 1)
        throw _Exception(_("Invalid value in ") + attrname + _(" for <") + 
                         xml->getName() + _("> tag"));

    return itmp;
}

void YouTubeService::addPagingParams(Ref<Element> xml, Ref<YouTubeTask> task)
{
    String temp;
    int itmp;
    temp = getCheckAttr(xml, _(CFG_OPTION_AMOUNT));
    if (temp == "all")
    {
        task->amount = AMOUNT_ALL;
        task->parameters->put(_(REST_PARAM_ITEMS_PER_PAGE),
                                    _(REST_VALUE_PER_PAGE_MAX));
    }
    else
    {
        itmp = getCheckPosIntAttr(xml, _(CFG_OPTION_AMOUNT));
        if (itmp >= _(REST_VALUE_PER_PAGE_MAX).toInt())
            task->parameters->put(_(REST_PARAM_ITEMS_PER_PAGE),
                    _(REST_VALUE_PER_PAGE_MAX));
        else
            task->parameters->put(_(REST_PARAM_ITEMS_PER_PAGE),
                    String::from(itmp));
        task->amount = itmp;
    }

    itmp = getCheckPosIntAttr(xml, _(CFG_OPTION_STARTPAGE));
    task->parameters->put(_(REST_PARAM_PAGE_NUMBER), 
            String::from(itmp));
}


Ref<Object> YouTubeService::defineServiceTask(Ref<Element> xmlopt)
{
    Ref<YouTubeTask> task(new YouTubeTask());
    String temp = xmlopt->getName();
    
    if (temp == CFG_METHOD_FAVORITES)
        task->method = YT_list_favorite;
    else if (temp == CFG_METHOD_TAG)
        task->method = YT_list_by_tag;
    else if (temp == CFG_METHOD_USER)
        task->method = YT_list_by_user;
    else if (temp == CFG_METHOD_FEATURED)
        task->method = YT_list_featured;
    else if (temp == CFG_METHOD_PLAYLIST)
        task->method = YT_list_by_playlist;
    else if (temp == CFG_METHOD_POPULAR)
        task->method = YT_list_popular;
    else if (temp == CFG_METHOD_CATEGORY_AND_TAG)
        task->method = YT_list_by_category_and_tag;
    else throw _Exception(_("Unsupported tag specified: ") + temp);

    if (!hasPaging(task->method))
        task->amount = AMOUNT_ALL;

    switch (task->method)
    {
        case YT_list_favorite:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_FAVORITE));

            task->parameters->put(_(REST_PARAM_USER), 
                                  getCheckAttr(xmlopt, _(CFG_OPTION_USER)));
            break;

        case YT_list_by_tag:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_BY_TAG));

            task->parameters->put(_(REST_PARAM_TAG), 
                                  getCheckAttr(xmlopt, _(CFG_OPTION_TAG)));

            addPagingParams(xmlopt, task);
            break;

        case YT_list_by_user:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_BY_USER));

            task->parameters->put(_(REST_PARAM_USER), 
                                  getCheckAttr(xmlopt, _(CFG_OPTION_USER)));

            addPagingParams(xmlopt, task);
            break;

        case YT_list_featured:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_FEATURED));
            break;

        case YT_list_by_playlist:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_BY_PLAYLIST));
            task->parameters->put(_(REST_PARAM_PLAYLIST_ID),
                               getCheckAttr(xmlopt, _(CFG_OPTION_PLAYLIST_ID)));
            addPagingParams(xmlopt, task); 
            
            break;
        case YT_list_popular:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_POPULAR));
            temp = getCheckAttr(xmlopt, _(CFG_OPTION_TIME_RANGE));
            if ((temp != REST_VALUE_TIME_RANGE_ALL) &&
                (temp != REST_VALUE_TIME_RANGE_DAY) &&
                (temp != REST_VALUE_TIME_RANGE_WEEK) &&
                (temp != REST_VALUE_TIME_RANGE_MONTH))
            {
                throw _Exception(_("Invalid time range specified for <") +
                                 xmlopt->getName() + _("< tag!"));
            }
            else 
                task->parameters->put(_(REST_PARAM_TIME_RANGE), temp);
            break;

        case YT_list_by_category_and_tag:
            task->parameters->put(_(REST_PARAM_METHOD),
                                  _(REST_METHOD_LIST_BY_CAT_AND_TAG));
            task->parameters->put(_(REST_PARAM_TAG),
                                  getCheckAttr(xmlopt, _(CFG_OPTION_TAG)));
           
            temp = getCheckAttr(xmlopt, _(CFG_OPTION_CATEGORY));
            if (temp == CFG_CAT_STRING_FILM_AND_ANIM)
            {
                task->category = YT_cat_film_and_animation;
                temp = String::from(YT_cat_film_and_animation);
            }
            else if (temp == CFG_CAT_STRING_AUTOS_AND_VEHICLES)
            {
                task->category = YT_cat_autos_and_vehicles;
                temp = String::from(YT_cat_autos_and_vehicles);
            }
            else if (temp == CFG_CAT_STRING_COMEDY)
            {
                task->category = YT_cat_comedy;
                temp = String::from(YT_cat_comedy);
            }
            else if (temp == CFG_CAT_STRING_ENTERTAINMENT)
            {
                task->category = YT_cat_entertainment;
                temp = String::from(YT_cat_entertainment);
            }
            else if (temp == CFG_CAT_STRING_MUSIC)
            {
                task->category = YT_cat_music;
                temp = String::from(YT_cat_music);
            }
            else if (temp == CFG_CAT_STRING_NEWS_AND_POLITICS)
            {
                task->category = YT_cat_news_and_politics;
                temp = String::from(YT_cat_news_and_politics);
            }
            else if (temp == CFG_CAT_STRING_PEOPLE_AND_BLOGS)
            {
                task->category = YT_cat_people_and_blogs;
                temp = String::from(YT_cat_people_and_blogs);
            }
            else if (temp == CFG_CAT_STRING_PETS_AND_ANIMALS)
            {
                task->category = YT_cat_pets_and_animals;
                temp = String::from(YT_cat_pets_and_animals);
            }
            else if (temp == CFG_CAT_STRING_HOWTO_AND_DIY)
            {
                task->category = YT_cat_howto_and_diy;
                temp = String::from(YT_cat_howto_and_diy);
            }
            else if (temp == CFG_CAT_STRING_SPORTS)
            {
                task->category = YT_cat_sports;
                temp = String::from(YT_cat_sports);
            }
            else if (temp == CFG_CAT_STRING_TRAVEL_AND_PLACES)
            {
                task->category = YT_cat_travel_and_places;
                temp = String::from(YT_cat_travel_and_places);
            }
            else if (temp == CFG_CAT_STRING_GADGETS_AND_GAMES)
            {
                task->category = YT_cat_gadgets_and_games;
                temp = String::from(YT_cat_gadgets_and_games);
            }
            else
            {
                throw _Exception(_("Invalid category specified for <") +
                        xmlopt->getName() + _("< tag!"));
            }
            task->parameters->put(_(REST_PARAM_CATEGORY_ID), temp);

            addPagingParams(xmlopt, task);
            break;

        case YT_list_none:
        default:
            throw _Exception(_("Unsupported tag!"));
            break;
    } // switch
    return RefCast(task, Object);
}

Ref<Element> YouTubeService::getData(Ref<Dictionary> params)
{
    long retcode;
    Ref<StringConverter> sc = StringConverter::i2i();

    // dev id is requied in each call
    String URL = _(REST_BASE_URL) + params->encode();

    log_debug("Retrieving URL: %s\n", URL.c_str());
   
    Ref<StringBuffer> buffer;
    try 
    {
        log_debug("DOWNLOADING URL: %s\n", URL.c_str());
        buffer = url->download(URL, &retcode, curl_handle, false, true);
    }
    catch (Exception ex)
    {
        log_error("Failed to download YouTube XML data: %s\n", 
                  ex.getMessage().c_str());
        return nil;
    }

    if (buffer == nil)
        return nil;

    if (retcode != 200)
        return nil;

    log_debug("GOT BUFFER\n%s\n", buffer->toString().c_str()); 
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

bool YouTubeService::hasPaging(yt_methods_t method)
{
    switch (method)
    {
        case YT_list_by_tag:
        case YT_list_by_user:
        case YT_list_by_playlist:
        case YT_list_by_category_and_tag:
            return true;

        case YT_list_popular:
        case YT_list_featured:
        case YT_list_none:
        case YT_list_favorite:
        default:
            return false;
    }
    return false;
}

bool YouTubeService::refreshServiceData(Ref<Layout> layout)
{
    log_debug("Refreshing YouTube service\n");
    // the layout is in full control of the service items
    
    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    // we do it here because the handle is initialized in a different thread
    // which is OK
    if (pid == 0)
        pid = pthread_self();

    if (pid != pthread_self())
        throw _Exception(_("Not allowed to call refreshServiceData from different threads!"));

    Ref<ConfigManager> config = ConfigManager::getInstance();
    Ref<Array<Object> > tasklist = config->getObjectArrayOption(CFG_ONLINE_CONTENT_YOUTUBE_TASK_LIST);

    if (tasklist->size() == 0)
        throw _Exception(_("Not specified what content to fetch!"));

    Ref<YouTubeTask> task = RefCast(tasklist->get(current_task), YouTubeTask);
    if (task == nil)
        throw _Exception(_("Encountered invalid task!"));


    if (hasPaging(task->method) && 
       ((task->amount == AMOUNT_ALL) || (task->amount > PER_PAGE_MAX)))
    {
        task->current_page++;
        task->parameters->put(_(REST_PARAM_PAGE_NUMBER),
                              String::from(task->current_page));
    }

    Ref<Element> reply = getData(task->parameters);

    Ref<YouTubeContentHandler> yt(new YouTubeContentHandler());
    bool b = false;
    if (reply != nil)
        b = yt->setServiceContent(reply);
    else
    {
        log_debug("Failed to get XML content from YouTube service\n");
        throw _Exception(_("Failed to get XML content from YouTube service"));
    }

    // no more items to fetch, reset paging and skip to next task
    if (!b)
    {
        log_debug("End of pages\n");
        if (hasPaging(task->method))
        {
            task->current_page = 0;
            task->amount_fetched = 0;
        }
        current_task++;
        if (current_task >= tasklist->size())
        {
            current_task = 0;
            return false;
        }
   
        return true;
    }

    /// \todo make sure the CdsResourceManager knows whats going on,
    /// since those items do not contain valid links but need to be
    /// processed later on (i.e. need to figure out the real link to the flv)
    Ref<CdsObject> obj;
    do
    {
        /// \todo add try/catch here and a possibility do find out if we
        /// may request more stuff or if we are at the end of the list
        obj = yt->getNextObject();
        if (obj == nil)
            break;

        obj->setVirtual(true);
        /// \todo we need a function that would do a lookup on the special
        /// service ID and tell is uf a particular object already exists
        /// in the database
        Ref<CdsObject> old = Storage::getInstance()->loadObjectByServiceID(RefCast(obj, CdsItem)->getServiceID());
//        Ref<CdsObject> old;
        if (old == nil)
        {
            log_debug("Adding new YouTube object\n");
            obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST), 
                            String::from(task->method));

            if (task->method == YT_list_by_category_and_tag)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_CATEGORY),
                        String::from(task->category));
            }
            
            if (layout != nil)
                layout->processCdsObject(obj);
        }
        else
        {
            log_debug("Updating existing YouTube object\n");
            obj->setID(old->getID());
            obj->setParentID(old->getParentID());
            struct timespec oldt, newt;
            oldt.tv_nsec = 0;
            oldt.tv_sec = old->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE)).toLong();
            newt.tv_nsec = 0;
            newt.tv_sec = obj->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE)).toLong();
            ContentManager::getInstance()->updateObject(obj);
        }

        if (task->amount != AMOUNT_ALL)
        {
            task->amount_fetched++;
            // max amount reached, reset paging and break to next task
            if (task->amount_fetched >= task->amount)
            {
                task->amount_fetched = 0;
                if (hasPaging(task->method))
                {
                    task->current_page = 0;
                }
                current_task++;
                if (current_task >= tasklist->size())
                {
                    current_task = 0;
                    return false;
                }
            }
        }

        if (Server::getInstance()->getShutdownStatus())
            return false;

    }
    while (obj != nil);

    if (!hasPaging(task->method))
    {
        current_task++;
        if (current_task >= tasklist->size())
        {
            current_task = 0;
            return false;
        }
    }

    return true;
}

#endif//YOUTUBE
