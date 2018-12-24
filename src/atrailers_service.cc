/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    atrailers_service.cc - this file is part of MediaTomb.
    
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

/// \file atrailers_service.cc

#ifdef ATRAILERS

#include "atrailers_service.h"
#include "atrailers_content_handler.h"
#include "config_manager.h"
#include "config_options.h"
#include "content_manager.h"
#include "server.h"
#include "string_converter.h"
#include "zmm/zmm.h"

#include <string>

using namespace zmm;
using namespace mxml;

#define ATRAILERS_SERVICE_URL_640 "http://www.apple.com/trailers/home/xml/current.xml"
#define ATRAILERS_SERVICE_URL_720P "http://www.apple.com/trailers/home/xml/current_720p.xml"

ATrailersService::ATrailersService()
{
    url = Ref<URL>(new URL());
    pid = 0;
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw _Exception(_("failed to initialize curl!\n"));

    if (ConfigManager::getInstance()->getOption(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION) == "640")
        service_url = _(ATRAILERS_SERVICE_URL_640);
    else
        service_url = _(ATRAILERS_SERVICE_URL_720P);
}

ATrailersService::~ATrailersService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

service_type_t ATrailersService::getServiceType()
{
    return OS_ATrailers;
}

String ATrailersService::getServiceName()
{
    return _("Apple Trailers");
}

Ref<Object> ATrailersService::defineServiceTask(Ref<Element> xmlopt, Ref<Object> params)
{
    // there are no configurable tasks here, we fetch an XML and parse it
    return nullptr;
}

Ref<Element> ATrailersService::getData()
{
    long retcode;
    Ref<StringConverter> sc = StringConverter::i2i();

    std::string buffer;

    try {
        log_debug("DOWNLOADING URL: %s\n", service_url.c_str());
        buffer = url->download(service_url, &retcode,
            curl_handle, false, true, true);
    } catch (const Exception& ex) {
        log_error("Failed to download Apple Trailers XML data: %s\n",
            ex.getMessage().c_str());
        return nullptr;
    }

    if (buffer.empty())
        return nullptr;

    if (retcode != 200)
        return nullptr;

    log_debug("GOT BUFFER\n%s\n", buffer.c_str());
    Ref<Parser> parser(new Parser());
    try {
        return parser->parseString(sc->convert(buffer))->getRoot();
    } catch (const ParseException& pe) {
        log_error("Error parsing Apple Trailers XML %s line %d:\n%s\n",
            pe.context->location.c_str(),
            pe.context->line,
            pe.getMessage().c_str());
        return nullptr;
    } catch (const Exception& ex) {
        log_error("Error parsing Apple Trailers XML %s\n",
            ex.getMessage().c_str());
        return nullptr;
    }

    return nullptr;
}

bool ATrailersService::refreshServiceData(Ref<Layout> layout)
{
    log_debug("Refreshing Apple Trailers\n");
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

    Ref<Element> reply = getData();

    Ref<ATrailersContentHandler> sc(new ATrailersContentHandler());
    if (reply != nullptr)
        sc->setServiceContent(reply);
    else {
        log_debug("Failed to get XML content from Trailers service\n");
        throw _Exception(_("Failed to get XML content from Trailers service"));
    }

    Ref<CdsObject> obj;
    do {
        obj = sc->getNextObject();
        if (obj == nullptr)
            break;

        obj->setVirtual(true);

        Ref<CdsObject> old = Storage::getInstance()->loadObjectByServiceID(RefCast(obj, CdsItem)->getServiceID());
        if (old == nullptr) {
            log_debug("Adding new Trailers object\n");

            if (layout != nullptr)
                layout->processCdsObject(obj, nullptr);
        } else {
            log_debug("Updating existing Trailers object\n");
            obj->setID(old->getID());
            obj->setParentID(old->getParentID());
            //            struct timespec oldt, newt;
            //            oldt.tv_nsec = 0;
            //            oldt.tv_sec = old->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE)).toLong();
            //            newt.tv_nsec = 0;
            //            newt.tv_sec = obj->getAuxData(_(ONLINE_SERVICE_LAST_UPDATE)).toLong();
            ContentManager::getInstance()->updateObject(obj);
        }

//        if (Server::getInstance()->getShutdownStatus())
//            return false;

    } while (obj != nullptr);

    return false;
}

#endif //ATRAILERS
