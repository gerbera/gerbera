/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    weborama_service.cc - this file is part of MediaTomb.
    
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

/// \file weborama_service.cc

#ifdef WEBORAMA 

#include "zmm/zmm.h"
#include "weborama_service.h"
#include "weborama_content_handler.h"
#include "content_manager.h"
#include "string_converter.h"
#include "config_manager.h"
#include "config_options.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

#define WEBORAMA_SERVICE_URL            "www.weborama.ru/modules/player/index_xspf.php"

/// \todo ask Nick if this value makes sense
#define DEFAULT_PER_TASK_AMOUNT         (1000)

#define WEBORAMA_VALUE_PER_REQUEST_MAX  (100)
// Weborama defines
#define FILTER_OPTION_FAVORITE          "favourite"
#define FILTER_OPTION_FRIENDS           "friends"
#define FILTER_OPTION_USERID            "userId"
#define FILTER_OPTION_UPLOADED          "uploaded"
//#define FILTER_OPTION_USER_COUNT        "user_count"
#define FILTER_OPTION_USER_LAST         "used_last"
#define FILTER_OPTION_ALL               "all"
#define FILTER_OPTION_GENREID           "genreId"
#define FILTER_OPTION_SORT              "sort"
#define FILTER_SEPARATOR                ";"
#define FILTER_VALUE_SEPARATOR          ":"


#define PARAM_LIMIT                     "limit"
#define PARAM_OFFSET                    "offset"
#define PARAM_COVER_SIZE                "coverSize"
#define PARAM_FILTER                    "filter"
#define PARAM_TYPE                      "type"
#define PARAM_MOOD                      "mood"
#define PARAM_ACTION                    "action"
#define PARAM_ID                        "id"

#define VALUE_COVER_SIZE_DLNA           "160"
#define VALUE_TYPE_PLAYLIST             "playlist"
#define VALUE_TYPE_AUDIO                "audio"

// config.xml defines
#define CFG_REQUEST_PLAYLIST            "playlist"

#define CFG_SECTION_FILTER              "filter"
#define CFG_SECTION_GENRES              "genres"
#define CFG_SECTION_SORT                "sort"
#define CFG_SECTION_TYPE                "type"

#define CFG_SORT_FAVORITE        "favorite"
#define CFG_SORT_FRIENDS         "friends"
#define CFG_SORT_UPLOADED        "uploaded"
#define CFG_SORT_USED_LAST       "used_last"
#define CFG_SORT_ALL             "all"

#define CFG_OPTION_PLAYLIST_NAME        "name"

#define CFG_MOOD_0               "calm-positive"     
#define CFG_MOOD_1               "positive"
#define CFG_MOOD_2               "energetic-positive"
#define CFG_MOOD_3               "calm"
#define CFG_MOOD_4               "neutral"
#define CFG_MOOD_5               "energetic"
#define CFG_MOOD_6               "calm-dark"
#define CFG_MOOD_7               "dark"
#define CFG_MOOD_8               "energetic-dark"

#define CFG_TYPE_AUDIO           "audio"
#define CFG_TYPE_PLAYLIST        "playlist"
#define CFG_TYPE_ALBUM           "album"

/*
Электронная       67
Русская попса     400
Альтернатива      60
Саундтрек         353
Джаз              65
Русский рок       315
Металл            272
Ретро             568
Поп               69
Панк              388
Ритм энд блюз     738
Рэп               593
Регги             699
Рок               68
Классическая      976
Шансон            566
Фолк              63
*/

#define CFG_GENRE_ELECT          "electronic"
#define CFG_GENRE_ALT            "alternative"
#define CFG_GENRE_JAZZ           "jazz"
#define CFG_GENRE_METAL          "metal"
#define CFG_GENRE_DMETAL         "death-metal"
#define CFG_GENRE_POP            "pop"
#define CFG_GENRE_PUNK           "punk"
#define CFG_GENRE_RNB            "rhytm-and-blues"
#define CFG_GENRE_RAP            "rap"
#define CFG_GENRE_REG            "reggae"
#define CFG_GENRE_ROCK           "rock"
#define CFG_GENRE_CLAS           "classic"
#define CFG_GENRE_ST             "soundtrack"
#define CFG_GENRE_RETRO          "retro"
#define CFG_GENRE_RUSROCK        "russian-rock"
#define CFG_GENRE_RUSPOP         "russian-pop"
#define CFG_GENRE_CHAN           "chanson"
#define CFG_GENRE_FOLK           "folk"

typedef struct wr_mood wr_mood;
struct wr_mood
{
    wr_mood_t mood;
    const char *cfg_name;
    const char *aux_name;
};

wr_mood WR_mood[] = 
{
    { WR_mood_calm_positive,         CFG_MOOD_0, "Calm-Positive"      },
    { WR_mood_positive,              CFG_MOOD_1, "Positive"           },
    { WR_mood_energetic_positive,    CFG_MOOD_2, "Energetic-Positive" },
    { WR_mood_calm,                  CFG_MOOD_3, "Calm"               },
    { WR_mood_neutral,               CFG_MOOD_4, "Neutral"            },
    { WR_mood_energetic,             CFG_MOOD_5, "Energetic"          },
    { WR_mood_calm_dark,             CFG_MOOD_6, "Calm-Dark"          },
    { WR_mood_dark,                  CFG_MOOD_7, "Dark"               },
    { WR_mood_energetic_dark,        CFG_MOOD_8, "Energetic-Dark"     },
    { WR_mood_none, NULL, NULL },
};

typedef struct wr_genre wr_genre;
struct wr_genre
{
    wr_genre_t genre;
    int id;
    const char *cfg_name;
    const char *aux_name;
};

wr_genre WR_genre[] = 
{   
    { WR_genre_electronic,      67,  CFG_GENRE_ELECT,   "Electronic"      },
    { WR_genre_alternative,     60,  CFG_GENRE_ALT,     "Alternative"     },
    { WR_genre_jazz,            65,  CFG_GENRE_JAZZ,    "Jazz"            }, 
    { WR_genre_metal,           181, CFG_GENRE_METAL,   "Metal"           },
    { WR_genre_death_metal,     549, CFG_GENRE_DMETAL,  "Death Metal"     }, 
    { WR_genre_pop,             69,  CFG_GENRE_POP,     "Pop"             },
    { WR_genre_punk,            388, CFG_GENRE_PUNK,    "Punk"            },
    { WR_genre_rhytm_and_blues, 738, CFG_GENRE_RNB,     "Rhytm And Blues" },
    { WR_genre_rap,             593, CFG_GENRE_RAP,     "Rap"             },
    { WR_genre_reggae,          699, CFG_GENRE_REG,     "Reggae"          },
    { WR_genre_rock,            68,  CFG_GENRE_ROCK,    "Rock"            },
    { WR_genre_classic,         976, CFG_GENRE_CLAS,    "Classic"         },
    { WR_genre_soundtrack,      353, CFG_GENRE_ST,      "Soundtrack"      },
    { WR_genre_retro,           568, CFG_GENRE_RETRO,   "Retro"           },
    { WR_genre_russian_rock,    315, CFG_GENRE_RUSROCK, "Russian Rock"    },
    { WR_genre_russian_pop,     400, CFG_GENRE_RUSPOP,  "Russian Pop"     },
    { WR_genre_chanson,         566, CFG_GENRE_CHAN,    "Chanson"         },
    { WR_genre_folk,            63,  CFG_GENRE_FOLK,    "Folk"            },
    { WR_genre_none, -1, NULL, NULL },                           
};

WeboramaService::WeboramaService()
{
    url = Ref<URL>(new URL());
    pid = 0;
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    current_task = 0;
}

WeboramaService::~WeboramaService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

WeboramaService::WeboramaTask::WeboramaTask()
{
    parameters = zmm::Ref<Dictionary>(new Dictionary());
    amount = 0;
    amount_fetched = 0;
    start_index = 0;
}


service_type_t WeboramaService::getServiceType()
{
    return OS_Weborama;
}

String WeboramaService::getServiceName()
{
    return _("Weborama");
}

wr_mood_t WeboramaService::getMood(Ref<Element> xml)
{
    String temp = xml->getAttribute(_("mood"));
    if (!string_ok(temp))
        return WR_mood_none;

    int i = temp.toInt();
    if ((i < 0) || (i >= WR_mood_none))
        throw _Exception(_("Weborama: invalid mood specified for tag <") + 
                         xml->getName() + ">");

    return WR_mood[i].mood;
}

wr_genre_t WeboramaService::getGenre(String genre)
{
    if (!string_ok(genre))
        return WR_genre_none;

    for (int i = (int)WR_genre_electronic; i < (int)WR_genre_none; i++)
    {
        if (genre == WR_genre[i].cfg_name)
            return WR_genre[i].genre;
    }

    return WR_genre_none;
}

int WeboramaService::getGenreID(wr_genre_t genre)
{
    if (((int)genre < 0) || ((int)genre > WR_genre_none))
        return -1;

    return WR_genre[genre].id;
}

Ref<Object> WeboramaService::defineServiceTask(Ref<Element> xmlopt, Ref<Object> params)
{
    Ref<WeboramaTask> task(new WeboramaTask());
    String temp = xmlopt->getName();
        
    task->parameters->put(_(PARAM_COVER_SIZE), _(VALUE_COVER_SIZE_DLNA));
    task->amount = DEFAULT_PER_TASK_AMOUNT;

    if (temp == CFG_REQUEST_PLAYLIST)
    {
        task->name = getCheckAttr(xmlopt, _(CFG_OPTION_PLAYLIST_NAME));

        Ref<Element> type = xmlopt->getChildByName(_(CFG_SECTION_TYPE));
        if (type != nil)
        {
            String id = type->getAttribute(_("id"));

            temp = type->getText();
            if (string_ok(temp))
            {
                if ((temp != CFG_TYPE_ALBUM) && (temp != CFG_TYPE_AUDIO) &&
                    (temp != CFG_TYPE_PLAYLIST))
                    throw _Exception(_("Weborama: ") + temp + 
                            _(" type is invalid!"));

                if (((temp == CFG_TYPE_ALBUM) || (temp == CFG_TYPE_AUDIO))
                        && (!string_ok(id) || id == "0"))
                {
                    throw _Exception(_("Weborama: ") + temp + 
                                     _(" type requires a valid id parameter!"));
                }

                task->parameters->put(_(PARAM_TYPE), temp);
                if (string_ok(id))
                    task->parameters->put(_(PARAM_ID), id);
            }
        }

        wr_mood_t mood = getMood(xmlopt);
        if (mood != WR_mood_none)
            task->parameters->put(_(PARAM_MOOD), String::from((int)mood));
        

        Ref<Element> filter = xmlopt->getChildByName(_(CFG_SECTION_FILTER));
        if (filter != nil)
        {
            String filters;
            String genres = filter->getChildText(_(CFG_SECTION_GENRES));
            if (string_ok(genres))
            {
                String g;
                Ref<Array<StringBase> > parts = split_string(genres, ',');
                for (int i = 0; i < parts->size(); i++)
                {
                    wr_genre_t genre = getGenre(parts->get(i));
                    if (genre == WR_genre_none)
                        continue;

                    g = g + String::from(getGenreID(genre)) + _(",");
                }

                if (string_ok(g))
                    filters = _(FILTER_OPTION_GENREID) + 
                              _(FILTER_VALUE_SEPARATOR) + 
                              g.substring(0, g.length()-1);
            }

            Ref<Element> sort_tag = filter->getChildByName(_("sort"));
            if (sort_tag != nil)
            {
                String user_id = sort_tag->getAttribute(_("user-id"));
                String sort = sort_tag->getText();
                if (string_ok(sort))
                {
                    if (!string_ok(user_id))
                        throw _Exception(_("Weborama: sort option requires a "
                                           "valid user-id setting!"));

                    if ((sort != CFG_SORT_FAVORITE)  &&
                        (sort != CFG_SORT_FRIENDS)   && 
                        (sort != CFG_SORT_UPLOADED)  &&
                        (sort != CFG_SORT_USED_LAST) && 
                        (sort != CFG_SORT_ALL))
                        throw _Exception(_("Weborama: ") + sort + 
                                         _(" sort is invalid!"));

                        // ok, this is a little hack'ish, but so far the config 
                        // names match nicely with the actual parameters... 
                        // all but one
                    if (sort == CFG_SORT_FAVORITE) 
                        sort = _(FILTER_OPTION_FAVORITE);

                    if (string_ok(filters))
                        filters = filters + _(FILTER_SEPARATOR);

                    filters = filters + _(FILTER_OPTION_SORT) + 
                             _(FILTER_VALUE_SEPARATOR) + sort +
                             _(FILTER_SEPARATOR) +
                             _(FILTER_OPTION_USERID) + user_id;
                }
            }

            if (string_ok(filters))
                task->parameters->put(_(PARAM_FILTER), filters);
        }

        temp = xmlopt->getAttribute(_("amount"));
        if (string_ok(temp))
        {
            int amount = temp.toInt();
            if (amount < 0)
                throw _Exception(_("Weborama: invalid amount specified for task ") + xmlopt->getName());

            task->amount = amount;
        }
        else
            task->amount = DEFAULT_PER_TASK_AMOUNT;
    } 
    else
        return nil;

    return RefCast(task, Object);
}

Ref<Element> WeboramaService::getData(Ref<Dictionary> params)
{
    long retcode;
    Ref<StringConverter> sc = StringConverter::i2i();

    Ref<StringBuffer> buffer;
    
    try 
    {
        String URL = _(WEBORAMA_SERVICE_URL) + _("?") + params->encode();
        log_debug("DOWNLOADING URL: %s\n", URL.c_str());
        buffer = url->download(URL, &retcode, 
                               curl_handle, false, true, true);
    }
    catch (Exception ex)
    {
        log_error("Failed to download Weborama XML data: %s\n", 
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
        return parser->parseString(sc->convert(buffer->toString()))->getRoot();
    }
    catch (ParseException pe)
    {
        log_error("Error parsing Weborama XML %s line %d:\n%s\n",
               pe.context->location.c_str(),
               pe.context->line,
               pe.getMessage().c_str());
        return nil;
    }
    catch (Exception ex)
    {
        log_error("Error parsing Weborama XML %s\n", ex.getMessage().c_str());
        return nil;
    }
    
    return nil;
}

bool WeboramaService::refreshServiceData(Ref<Layout> layout)
{
    log_debug("Refreshing Weborama service\n");
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
    Ref<Array<Object> > tasklist = config->getObjectArrayOption(CFG_ONLINE_CONTENT_WEBORAMA_TASK_LIST);

    if (tasklist->size() == 0)
        throw _Exception(_("Not specified what content to fetch!"));

    Ref<WeboramaTask> task = RefCast(tasklist->get(current_task), WeboramaTask);
    if (task ==  nil)
        throw _Exception(_("Encountered invalid task!"));

    if (task->amount_fetched < task->amount)
    {
        task->start_index = task->amount_fetched;
        task->parameters->put(_(PARAM_OFFSET), String::from(task->start_index));
        if ((task->amount - task->amount_fetched) >=
                WEBORAMA_VALUE_PER_REQUEST_MAX)
        {
            task->parameters->put(_(PARAM_LIMIT), 
                                  String::from(WEBORAMA_VALUE_PER_REQUEST_MAX));
        }
        else if ((task->amount - task->amount_fetched) <
                 WEBORAMA_VALUE_PER_REQUEST_MAX)
        {
             task->parameters->put(_(PARAM_LIMIT), 
                     String::from(task->amount - task->amount_fetched));
        }
    }
    bool b = false;

    Ref<Element> reply = getData(task->parameters);

    Ref<WeboramaContentHandler> sc(new WeboramaContentHandler());
    if (reply != nil)
        b = sc->setServiceContent(reply);
    else
    {
        log_debug("Failed to get XML content from Weborama service\n");
        throw _Exception(_("Failed to get XML content from Weborama service"));
    }

    if (!b)
    {
        log_debug("End of pages\n");
        task->amount_fetched = 0;
        task->start_index = 0;

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
        obj = sc->getNextObject();
        if (obj == nil)
        {
            if (task->amount_fetched < task->amount)
                return true;
            else
                break;
        }

        obj->setVirtual(true);
        obj->setAuxData(_(WEBORAMA_AUXDATA_REQUEST_NAME), task->name);
        RefCast(obj, 
                CdsItemExternalURL)->setTrackNumber(task->amount_fetched+1);

        Ref<CdsObject> old = Storage::getInstance()->loadObjectByServiceID(RefCast(obj, CdsItem)->getServiceID());
        if (old == nil)
        {
            log_debug("Adding new Weborama object\n");
            
            if (layout != nil)
                layout->processCdsObject(obj, nil);
        }
        else
        {
            log_debug("Updating existing Weborama object\n");
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
        if (task->amount_fetched >= task->amount)
        {
            task->amount_fetched = 0;
            task->start_index = 0;

            current_task++;
            if (current_task >= tasklist->size())
            {
                current_task = 0;
                return false;
            }
        }

        if (Server::getInstance()->getShutdownStatus())
            return false;

    }
    while (obj != nil);

    current_task++;
    if (current_task >= tasklist->size())
    {
        current_task = 0;
        return false;
    }

    return false;
}

#endif//WEBORAMA
