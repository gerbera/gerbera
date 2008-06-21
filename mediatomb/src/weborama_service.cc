/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    weborama_service.cc - this file is part of MediaTomb.
    
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
    
    $Id: weborama_service.cc 1811 2008-05-14 16:48:32Z jin_eld $
*/

/// \file weborama_service.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef WEBORAMA 

#include "zmm/zmm.h"
#include "weborama_service.h"
#include "weborama_content_handler.h"
#include "content_manager.h"
#include "string_converter.h"
#include "config_manager.h"
#include "server.h"

using namespace zmm;
using namespace mxml;

#define WEBORAMA_SERVICE_URL            "http://beta.weborama.ru/modules/index_xspf.php"
#define MAX_PER_TASK_AMOUNT             (100)

// Weborama defines
#define FILTER_OPTION_FAVORITE          "favourite"
#define FILTER_OPTION_FRIENDS           "friends"
#define FILTER_OPTION_USERID            "userId"
#define FILTER_OPTION_UPLOADED          "uploaded"
#define FILTER_OPTION_USER_COUNT        "user_count"
#define FILTER_OPTION_USER_LAST         "user_last"
#define FILTER_OPTION_ALL               "all"
#define FILTER_SEPARATOR                ":"

#define PARAM_LIMIT                     "limit"
#define PARAM_OFFSET                    "offset"
#define PARAM_COVER_SIZE                "coverSize"
#define PARAM_FILTER                    "filter"
#define PARAM_TYPE                      "type"

#define VALUE_COVER_SIZE_DLNA           "160"
#define VALUE_TYPE_PLAYLIST             "playlist"
#define VALUE_TYPE_AUDIO                "audio"

// config.xml defines
#define CFG_REQUEST_FAVORITES           "favorites"

#define CFG_OPTION_USER                 "user"

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
    cfg_start_index = 0;
}


service_type_t WeboramaService::getServiceType()
{
    return OS_Weborama;
}

String WeboramaService::getServiceName()
{
    return _("Weborama");
}

Ref<Object> WeboramaService::defineServiceTask(Ref<Element> xmlopt, Ref<Object> params)
{
    Ref<WeboramaTask> task(new WeboramaTask());
    String temp = xmlopt->getName();
    String temp2;

    if (temp == CFG_REQUEST_FAVORITES)
    {
        task->parameters->put(_(PARAM_COVER_SIZE), _(VALUE_COVER_SIZE_DLNA));
        task->parameters->put(_(PARAM_TYPE), _(VALUE_TYPE_PLAYLIST));

        temp2 = xmlopt->getAttribute(_("user"));
        if (!string_ok(temp))
            throw _Exception(_("Weborama: Missing \"user\" attribute for the <favorites> tag!"));

        temp2 = _(FILTER_OPTION_USERID) + _(FILTER_SEPARATOR) + temp2 + 
                _(FILTER_SEPARATOR) + _(FILTER_OPTION_FAVORITE);

        task->parameters->put(_(PARAM_FILTER), temp2);

        temp2 = xmlopt->getAttribute(_("amount"));
        if (string_ok(temp2))
        {
            int amount = temp2.toInt();
            if (amount < 0)
                throw _Exception(_("Weborama: invalid amount specified for task ") + xmlopt->getName());

            task->amount = amount;
        }
        else
            task->amount = MAX_PER_TASK_AMOUNT;
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
        return parser->parseString(sc->convert(buffer->toString()));
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
        obj = sc->getNextObject();
        if (obj == nil)
            break;

       obj->setVirtual(true);

        Ref<CdsObject> old = Storage::getInstance()->loadObjectByServiceID(RefCast(obj, CdsItem)->getServiceID());
        if (old == nil)
        {
            log_debug("Adding new Weborama object\n");
            
            if (layout != nil)
                layout->processCdsObject(obj);
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
