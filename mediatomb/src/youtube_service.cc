/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    youtube_service.cc - this file is part of MediaTomb.
    
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

// Base request URL
#define GDATA_API_YT_BASE_URL           "http://gdata.youtube.com/feeds/api/"

// /users/USERNAME/favorites
#define GDATA_REQUEST_FAVORITES         "/users/%s/favorites"

// /feeds/api/videos?vq="SEARCH TERMS"
#define GDATA_REQUEST_SEARCH_BY_TAG     "/videos"

// /feeds/api/users/USERNAME/uploads
#define GDATA_REQUEST_LIST_BY_USER      "/users/%s/uploads"

// all stdfeeds:  "/standardfeeds/REGION_ID/feed"
#define GDATA_REQUEST_STDFEED_BASE              "standardfeeds"

#define GDATA_REQUEST_STDFEED_TOP_RATED         "top_rated"
#define GDATA_REQUEST_STDFEED_TOP_FAVORITES     "top_favorites"
#define GDATA_REQUEST_STDFEED_MOST_VIEWED       "most_viewed"
#define GDATA_REQUEST_STDFEED_MOST_RECENT       "most_recent"
#define GDATA_REQUEST_STDFEED_MOST_DISCUSSED    "most_discussed"
#define GDATA_REQUEST_STDFEED_MOST_LINKED       "most_linked"
#define GDATA_REQUEST_STDFEED_MOST_RESPONDED    "most_responded"
#define GDATA_REQUEST_STDFEED_RECENTLY_FEATURED "recently_featured"
#define GDATA_REQUEST_STDFEED_WATCH_ON_MOBILE   "watch_on_mobile"

static char *YT_stdfeeds[] = 
{
    GDATA_REQUEST_STDFEED_TOP_RATED,        // has time
    GDATA_REQUEST_STDFEED_TOP_FAVORITES,    // has time
    GDATA_REQUEST_STDFEED_MOST_VIEWED,      // has time
    GDATA_REQUEST_STDFEED_MOST_RECENT,      
    GDATA_REQUEST_STDFEED_MOST_DISCUSSED,   // has time
    GDATA_REQUEST_STDFEED_MOST_LINKED,      // has time
    GDATA_REQUEST_STDFEED_MOST_RESPONDED,   // has time
    GDATA_REQUEST_STDFEED_RECENTLY_FEATURED,
    GDATA_REQUEST_STDFEED_WATCH_ON_MOBILE,
    NULL,
};

// gdata default parameters
//http://code.google.com/apis/youtube/reference.html#Query_parameter_definitions
#define GDATA_PARAM_FEED_FORMAT             "alt"
#define GDATA_PARAM_AUTHOR                  "author"
//#define GDATA_PARAM_MAX_RESULTS             "max-results"
//#define GDATA_PARAM_START_INDEX             "start-index"

// additional YT specific parameters
#define GDATA_YT_PARAM_SEARCH_TERM          "vq" // value must be url escaped
#define GDATA_YT_PARAM_ORDERBY              "orderby"
#define GDATA_YT_PARAM_FORMAT               "format"
#define GDATA_YT_PARAM_RESTRICT_LANGUAGE    "lr"
#define GDATA_YT_PARAM_RESTRICTED_CONTENT   "racy"
#define GDATA_YT_PARAM_COUNTRY_RESTRICTION  "restriction"
#define GDATA_YT_PARAM_TIME                 "time"

#define GDATA_YT_PARAM_START_INDEX          "start-index"
#define GDATA_YT_PARAM_MAX_RESULTS_PER_REQ  "max-results"

// allowed parameter values
#define GDATA_VALUE_FEED_FORMAT_RSS         "rss"

#define GDATA_YT_VALUE_ORDERBY_RELEVANCE    "relevance"
#define GDATA_YT_VALUE_ORDERBY_PUBLISHED    "published"
#define GDATA_YT_VALUE_ORDERBY_VIEWCOUNT    "viewCount"
#define GDATA_YT_VALUE_ORDERBY_RATING       "rating"
/// \todo relevance_lang_languageCode

#define GDATA_YT_VALUE_FORMAT_SWF           "5"
/// \todo do we need rtsp stuff? resolution is quite small there

// time range values
#define GDATA_YT_VALUE_TIME_RANGE_ALL       "all_time"
#define GDATA_YT_VALUE_TIME_RANGE_DAY       "today"
#define GDATA_YT_VALUE_TIME_RANGE_WEEK      "this_week"
#define GDATA_YT_VALUE_TIME_RANGE_MONTH     "this_month"

// REST API min/max items per page values
#define GDATA_YT_VALUE_START_INDEX_MIN      "1"
#define GDATA_YT_VALUE_PER_PAGE_MAX         "50"
#define AMOUNT_ALL                          (-333)

#define CAT_NAME_FILM                       "Film & Animation"
#define CAT_NAME_AUTOS                      "Autos & Vehicles"
#define CAT_NAME_COMEDY                     "Comedy"
#define CAT_NAME_ENTERTAINMENT              "Entertainment"
#define CAT_NAME_MUSIC                      "Music"
#define CAT_NAME_NEWS                       "News & Politics"
#define CAT_NAME_PEOPLE                     "People & Blogs"
#define CAT_NAME_ANIMALS                    "Pets & Animals"
#define CAT_NAME_HOWTO                      "Howto & Style" 
#define CAT_NAME_SPORTS                     "Sports"
#define CAT_NAME_TRAVEL                     "Travel & Events"
#define CAT_NAME_GADGETS                    "Gadgets & Games"
#define CAT_NAME_SHORT_MOVIES               "Short Movies"
#define CAT_NAME_VIDEOBLOG                  "Videoblogging"
#define CAT_NAME_EDUCATION                  "Education"
#define CAT_NAME_NONPROFIT                  "Nonprofits & Activism"

#define REQ_NAME_STDFEEDS                   "Standard Feeds"

#define REQ_NAME_FAVORITES                  "Favorites"
#define REQ_NAME_FEATURED                   "Featured"
#define REQ_NAME_POPULAR                    "Popular"
#define REQ_NAME_PLAYLIST                   "Playlists"
#define REQ_NAME_CATEGORY_AND_TAG           "Categories"
// custom names
#define REQ_NAME_BY_USER                    "User"
#define REQ_NAME_BY_TAG                     "Tag"

// config.xml defines
#define CFG_CAT_TERM_FILM                   "Film"
#define CFG_CAT_TERM_AUTOS                  "Autos"
#define CFG_CAT_TERM_MUSIC                  "Music"
#define CFG_CAT_TERM_ANIMALS                "Animals"
#define CFG_CAT_TERM_SPORTS                 "Sports"
#define CFG_CAT_TERM_TRAVEL                 "Travel"
#define CFG_CAT_TERM_SHORT_MOVIES           "Shortmov"
#define CFG_CAT_TERM_VIDEOBLOG              "Videoblog"
#define CFG_CAT_TERM_GADGETS                "Games"
#define CFG_CAT_TERM_COMEDY                 "Comedy"
#define CFG_CAT_TERM_PEOPLE                 "People"
#define CFG_CAT_TERM_NEWS                   "News"
#define CFG_CAT_TERM_ENTERTAINMENT          "Entertainment"
#define CFG_CAT_TERM_EDUCATION              "Education"
#define CFG_CAT_TERM_HOWTO                  "Howto"
#define CFG_CAT_TERM_NONPROFIT              "Nonprofit"
#define CFG_CAT_TERM_TECH                   "Tech"

#define CFG_REQUEST_STDFEED                 "standardfeed"

#define CFG_OPTION_USER                     "user"
#define CFG_OPTION_TAG                      "tag"
#define CFG_OPTION_STARTINDEX               "start-index"
#define CFG_OPTION_AMOUNT                   "amount"
#define CFG_OPTION_PLAYLIST_ID              "id"
#define CFG_OPTION_PLAYLIST_NAME            "name"
#define CFG_OPTION_TIME_RANGE               "time-range"
#define CFG_OPTION_CATEGORY                 "category"

#define CFG_OPTION_STDFEED                  "feed"
#define CFG_OPTION_REGION_ID                "region-id"

#define CFG_OPTION_REGION_AUSTRALIA         "au"
#define CFG_OPTION_REGION_BRAZIL            "br"
#define CFG_OPTION_REGION_CANADA            "ca"
#define CFG_OPTION_REGION_FRANCE            "fr"
#define CFG_OPTION_REGION_GERMANY           "de"
#define CFG_OPTION_REGION_GREAT_BRITAIN     "gb"
#define CFG_OPTION_REGION_HOLLAND           "nl"
#define CFG_OPTION_REGION_HONG_KONG         "hk"
#define CFG_OPTION_REGION_IRELAND           "ie"
#define CFG_OPTION_REGION_ITALY             "it"
#define CFG_OPTION_REGION_JAPAN             "jp"
#define CFG_OPTION_REGION_MEXICO            "mx"
#define CFG_OPTION_REGION_NEW_ZEALAND       "nz"
#define CFG_OPTION_REGION_POLAND            "pl"
#define CFG_OPTION_RUSSIA                   "ru"
#define CFG_OPTION_SOUTH_KOREA              "kr"
#define CFG_OPTION_SPAIN                    "es"
#define CFG_OPTION_TAIWAN                   "tw"
#define CFG_OPTION_UNITED_STATES            "us"

static char *YT_regions[] = 
{    
    CFG_OPTION_REGION_AUSTRALIA,
    CFG_OPTION_REGION_BRAZIL,
    CFG_OPTION_REGION_CANADA,
    CFG_OPTION_REGION_FRANCE,
    CFG_OPTION_REGION_GERMANY,
    CFG_OPTION_REGION_GREAT_BRITAIN,
    CFG_OPTION_REGION_HOLLAND,
    CFG_OPTION_REGION_HONG_KONG,
    CFG_OPTION_REGION_IRELAND,
    CFG_OPTION_REGION_ITALY,
    CFG_OPTION_REGION_JAPAN,
    CFG_OPTION_REGION_MEXICO,
    CFG_OPTION_REGION_NEW_ZEALAND,
    CFG_OPTION_REGION_POLAND,
    CFG_OPTION_RUSSIA,
    CFG_OPTION_SOUTH_KOREA,
    CFG_OPTION_SPAIN,
    CFG_OPTION_TAIWAN,
    CFG_OPTION_UNITED_STATES,
    NULL,
};

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
#warning ДОДЕЛАТЬ YT!
//    parameters->put(_(REST_PARAM_DEV_ID), ConfigManager::getInstance()->getOption(CFG_ONLINE_CONTENT_YOUTUBE_DEV_ID));
    request = YT_request_none;
    category = YT_cat_none;
    amount = 0;
    amount_fetched = 0;
    start_index = 1;
    cfg_start_index = 1;
}


service_type_t YouTubeService::getServiceType()
{
    return OS_YouTube;
}

String YouTubeService::getServiceName()
{
    return _("YouTube");
}

String YouTubeService::getRequestName(yt_requests_t request)
{
    String temp;

    switch (request)
    {
        case YT_request_stdfeed:
            temp = _(REQ_NAME_STDFEEDS);
            break;
#if 0
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
        case YT_list_by_tag:
            temp = _(REQ_NAME_BY_TAG);
            break;
        case YT_list_by_user:
            temp = _(REQ_NAME_BY_USER);
            break;
        case YT_list_none:
#endif
        default:
            temp = nil;
            break;
    }

    return temp;
}

String YouTubeService::getCategoryName(yt_categories_t category)
{
    return nil;
#warning ДОДЕЛАТЬ YT!

#if 0
    String temp;

    switch (category)
    {
        case YT_cat_film_and_animation:
            temp = _(CAT_NAME_FILM);
            break;
        case YT_cat_autos_and_vehicles:
            temp = _(CAT_NAME_AUTOS);
            break;
        case YT_cat_music:
            temp = _(CAT_NAME_MUSIC);
            break;
        case YT_cat_pets_and_animals:
            temp = _(CAT_NAME_ANIMALS);
            break;
        case YT_cat_sports:
            temp = _(CAT_NAME_SPORTS);
            break;
        case YT_cat_travel_and_places:
            temp = _(CAT_NAME_TRAVEL);
            break;
        case YT_cat_gadgets_and_games:
            temp = _(CAT_NAME_GADGETS);
            break;
        case YT_cat_people_and_blogs:
            temp = _(CAT_NAME_PEOPLE);
            break;
        case YT_cat_comedy:
            temp = _(CAT_NAME_COMEDY);
            break;
        case YT_cat_entertainment:
            temp = _(CAT_NAME_ENTERTAINMENT);
            break;
        case YT_cat_news_and_politics:
            temp = _(CAT_NAME_NEWS);
            break;
        case YT_cat_howto_and_diy:
            temp = _(CAT_NAME_HOWTO);
            break;
        case YT_cat_none:
        default:
            temp = nil;
            break;
    }

    return temp;
#endif
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

void YouTubeService::getPagingParams(Ref<Element> xml, Ref<YouTubeTask> task)
{
    String temp;
    int itmp;
    temp = xml->getAttribute(_(CFG_OPTION_AMOUNT));
    if (!string_ok(temp))
        temp = _("all");

    if (temp == "all")
        task->amount = AMOUNT_ALL;
    else
    {
        itmp = getCheckPosIntAttr(xml, _(CFG_OPTION_AMOUNT));
        task->amount = itmp;
    }

    temp = xml->getAttribute(_(CFG_OPTION_STARTINDEX));
    if (!string_ok(temp))
        itmp = 1;
    else
    {
        itmp = getCheckPosIntAttr(xml, _(CFG_OPTION_STARTINDEX));
        if (itmp <= 0)
        {
            throw _Exception(_("Tag <") + xml->getName() + _("> ") + 
                    _(CFG_OPTION_STARTINDEX) + _(" must be at least 1!"));
        }
    }
    task->cfg_start_index = itmp;
    task->start_index = itmp;
}

void YouTubeService::addTimeParams(Ref<Element> xml, Ref<YouTubeTask> task)
{
    String temp;
    
    temp = xml->getAttribute(_(CFG_OPTION_TIME_RANGE));
    if (!string_ok(temp))
        return;

    if ((temp != GDATA_YT_VALUE_TIME_RANGE_ALL) &&
        (temp != GDATA_YT_VALUE_TIME_RANGE_DAY) &&
        (temp != GDATA_YT_VALUE_TIME_RANGE_WEEK) &&
        (temp != GDATA_YT_VALUE_TIME_RANGE_MONTH))
        throw _Exception(_("Invalid time range parameter \"") + temp + 
                _("\" in <") + xml->getName() + _("> tag"));

    task->parameters->put(_(GDATA_YT_PARAM_TIME), temp);
}

String YouTubeService::getRegion(Ref<Element> xml)
{
    String region = xml->getAttribute(_(CFG_OPTION_REGION_ID));
    if (!string_ok(region))
        return nil;

    int count = 0;
    while (YT_regions[count] != NULL)
    {
        if (region == YT_regions[count])
            return region;
        count++;
    }

    throw _Exception(_("<") + xml->getName() + _("> tag has an invalid region setting: ") + region);

}

String YouTubeService::getFeed(Ref<Element> xml)
{
    String feed = xml->getAttribute(_(CFG_OPTION_STDFEED));
    if (!string_ok(feed))
        throw _Exception(_("<") + xml->getName() + _("> tag is missing the required feed setting!"));

    int count = 0;
    while (YT_stdfeeds[count] != NULL)
    {
        if (feed == YT_stdfeeds[count])
            return feed;
        count++;
    }

    throw _Exception(_("<") + xml->getName() + _("> tag has an invalid feed setting: ") + feed);

}
Ref<Object> YouTubeService::defineServiceTask(Ref<Element> xmlopt)
{
    Ref<YouTubeTask> task(new YouTubeTask());
    String temp = xmlopt->getName();
    String temp2;
    
    if (temp == CFG_REQUEST_STDFEED)
        task->request = YT_request_stdfeed;
    else throw _Exception(_("Unsupported tag while parsing YouTube options: ") + temp);

    if (!hasPaging(task->request))
        task->amount = AMOUNT_ALL;

    task->parameters->put(_(GDATA_PARAM_FEED_FORMAT), 
                          _(GDATA_VALUE_FEED_FORMAT_RSS));
    task->parameters->put(_(GDATA_YT_PARAM_FORMAT),
                          _(GDATA_YT_VALUE_FORMAT_SWF));
    switch (task->request)
    {
        case YT_request_stdfeed:
            temp2 = getFeed(xmlopt);
            task->url_part = _(GDATA_REQUEST_STDFEED_BASE) + _("/");

            if (temp2 != GDATA_REQUEST_STDFEED_WATCH_ON_MOBILE)
            {
                temp = getRegion(xmlopt);
                if (string_ok(temp))
                    task->url_part = task->url_part + temp + _("/");
            }

            task->url_part = task->url_part + temp2;
            if ((temp2 != GDATA_REQUEST_STDFEED_MOST_RECENT) &&
                (temp2 != GDATA_REQUEST_STDFEED_RECENTLY_FEATURED) &&
                (temp2 != GDATA_REQUEST_STDFEED_WATCH_ON_MOBILE))
            {
                addTimeParams(xmlopt, task);
            }

            getPagingParams(xmlopt, task);
            break;

#if 0
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
            if (temp == CFG_CAT_TERM_FILM)
            {
                task->category = YT_cat_film_and_animation;
                temp = String::from(YT_cat_film_and_animation);
            }
            else if (temp == CFG_CAT_TERM_AUTOS)
            {
                task->category = YT_cat_autos_and_vehicles;
                temp = String::from(YT_cat_autos_and_vehicles);
            }
            else if (temp == CFG_CAT_TERM_COMEDY)
            {
                task->category = YT_cat_comedy;
                temp = String::from(YT_cat_comedy);
            }
            else if (temp == CFG_CAT_TERM_ENTERTAINMENT)
            {
                task->category = YT_cat_entertainment;
                temp = String::from(YT_cat_entertainment);
            }
            else if (temp == CFG_CAT_TERM_MUSIC)
            {
                task->category = YT_cat_music;
                temp = String::from(YT_cat_music);
            }
            else if (temp == CFG_CAT_TERM_NEWS)
            {
                task->category = YT_cat_news_and_politics;
                temp = String::from(YT_cat_news_and_politics);
            }
            else if (temp == CFG_CAT_TERM_PEOPLE)
            {
                task->category = YT_cat_people_and_blogs;
                temp = String::from(YT_cat_people_and_blogs);
            }
            else if (temp == CFG_CAT_TERM_ANIMALS)
            {
                task->category = YT_cat_pets_and_animals;
                temp = String::from(YT_cat_pets_and_animals);
            }
            else if (temp == CFG_CAT_TERM_HOWTO)
            {
                task->category = YT_cat_howto_and_diy;
                temp = String::from(YT_cat_howto_and_diy);
            }
            else if (temp == CFG_CAT_TERM_SPORTS)
            {
                task->category = YT_cat_sports;
                temp = String::from(YT_cat_sports);
            }
            else if (temp == CFG_CAT_TERM_TRAVEL)
            {
                task->category = YT_cat_travel_and_places;
                temp = String::from(YT_cat_travel_and_places);
            }
            else if (temp == CFG_CAT_TERM_GADGETS)
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
#endif
        case YT_request_none:
        default:
            throw _Exception(_("Unsupported tag!"));
            break;
    } // switch
    return RefCast(task, Object);
}

Ref<Element> YouTubeService::getData(String url_part, Ref<Dictionary> params)
{
    long retcode;
    Ref<StringConverter> sc = StringConverter::i2i();

    String URL = _(GDATA_API_YT_BASE_URL) + url_part + _("?") + params->encode();

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

bool YouTubeService::hasPaging(yt_requests_t request)
{
    /*
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
    */
    return true;
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

    if (hasPaging(task->request) && 
       ((task->amount == AMOUNT_ALL) || (task->amount < task->amount_fetched)))
    {
        // yt start index begins at 1
        task->start_index = task->cfg_start_index + task->amount_fetched;
        task->parameters->put(_(GDATA_YT_PARAM_START_INDEX),
                              String::from(task->start_index));

        if ((task->amount == AMOUNT_ALL) || 
            ((task->amount - task->amount_fetched) > 
             _(GDATA_YT_VALUE_PER_PAGE_MAX).toInt()))
        {
           task->parameters->put(_(GDATA_YT_PARAM_MAX_RESULTS_PER_REQ),
                                 _(GDATA_YT_VALUE_PER_PAGE_MAX));
        }
        else if ((task->amount - task->amount_fetched) < 
                _(GDATA_YT_VALUE_PER_PAGE_MAX).toInt())
        {
            task->parameters->put(_(GDATA_YT_PARAM_MAX_RESULTS_PER_REQ),
                    String::from(task->amount - task->amount_fetched));
        }
    }

    Ref<Element> reply = getData(task->url_part, task->parameters);

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
        if (hasPaging(task->request))
        {
            task->start_index = task->cfg_start_index;
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

        Ref<CdsObject> old = Storage::getInstance()->loadObjectByServiceID(RefCast(obj, CdsItem)->getServiceID());
        if (old == nil)
        {
            log_debug("Adding new YouTube object\n");
            obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST), 
                            String::from(task->request));
#warning ДОДЕЛАТЬ YT
#if 0
            if (task->method == YT_list_by_category_and_tag)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST_SUBNAME),
                        String::from(task->category));
            }
            else if (task->method == YT_list_by_user)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST_SUBNAME), 
                        task->parameters->get(_(REST_PARAM_USER)));
            }
            else if (task->method == YT_list_favorite)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST_SUBNAME),
                        task->parameters->get(_(REST_PARAM_USER)));
            }
            else if (task->method == YT_list_by_playlist)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST_SUBNAME), 
                        task->playlist_name);
            }
            else if (task->method == YT_list_by_tag)
            {
                obj->setAuxData(_(YOUTUBE_AUXDATA_REQUEST_SUBNAME),
                        task->parameters->get(_(REST_PARAM_TAG)));
            }
#endif
            if (layout != nil)
            {
                printf("Нашел объект!!!!!!!!!!!!\n");
                layout->processCdsObject(obj);
            }
            else
                printf("---------------> NO OBJECTS PARSED!\n");
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

        task->amount_fetched++;
        if (task->amount != AMOUNT_ALL)
        {
            // max amount reached, reset paging and break to next task
            if (task->amount_fetched >= task->amount)
            {
                task->amount_fetched = 0;
                if (hasPaging(task->request))
                {
                    task->start_index = task->cfg_start_index;
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

    if (!hasPaging(task->request))
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
