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
#include "storage/storage.h"
#include "config/config_manager.h"
#include "config/config_options.h"
#include "content_manager.h"
#include "server.h"
#include "util/string_converter.h"

#include <string>

#define ATRAILERS_SERVICE_URL_640 "http://www.apple.com/trailers/home/xml/current.xml"
#define ATRAILERS_SERVICE_URL_720P "http://www.apple.com/trailers/home/xml/current_720p.xml"

ATrailersService::ATrailersService(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content)
    : config(config)
    , storage(storage)
    , content(content)
{
    pid = 0;
    curl_handle = curl_easy_init();
    if (!curl_handle)
        throw std::runtime_error("failed to initialize curl!\n");

    if (config->getOption(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION) == "640")
        service_url = ATRAILERS_SERVICE_URL_640;
    else
        service_url = ATRAILERS_SERVICE_URL_720P;
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

std::string ATrailersService::getServiceName()
{
    return "Apple Trailers";
}

std::unique_ptr<pugi::xml_document> ATrailersService::getData()
{
    long retcode;
    auto sc = StringConverter::i2i(config);

    std::string buffer;

    try {
        log_debug("DOWNLOADING URL: {}", service_url.c_str());
        buffer = URL::download(service_url, &retcode,
            curl_handle, false, true, true);
    } catch (const std::runtime_error& ex) {
        log_error("Failed to download Apple Trailers XML data: {}",
            ex.what());
        return nullptr;
    }

    if (buffer.empty())
        return nullptr;

    if (retcode != 200)
        return nullptr;

    log_debug("GOT BUFFER{}", buffer.c_str());
    auto doc = std::make_unique<pugi::xml_document>();
    pugi::xml_parse_result result = doc->load_string(sc->convert(buffer).c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        log_error("Error parsing Apple Trailers XML: {}", result.description());
        return nullptr;
    }

    return doc;
}

bool ATrailersService::refreshServiceData(std::shared_ptr<Layout> layout)
{
    log_debug("Refreshing Apple Trailers");
    // the layout is in full control of the service items

    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    // we do it here because the handle is initialized in a different thread
    // which is OK
    if (pid == 0)
        pid = pthread_self();

    if (pid != pthread_self())
        throw std::runtime_error("Not allowed to call refreshServiceData from different threads!");

    auto reply = getData();
    if (reply == nullptr) {
        log_debug("Failed to get XML content from Trailers service");
        throw std::runtime_error("Failed to get XML content from Trailers service");
    }

    auto sc = std::make_unique<ATrailersContentHandler>(config, storage);
    sc->setServiceContent(reply);

    std::shared_ptr<CdsObject> obj;
    do {
        obj = sc->getNextObject();
        if (obj == nullptr)
            break;

        obj->setVirtual(true);

        auto old = storage->loadObjectByServiceID(std::static_pointer_cast<CdsItem>(obj)->getServiceID());
        if (old == nullptr) {
            log_debug("Adding new Trailers object");

            if (layout != nullptr)
                layout->processCdsObject(obj, nullptr);
        } else {
            log_debug("Updating existing Trailers object");
            obj->setID(old->getID());
            obj->setParentID(old->getParentID());
            //            struct timespec oldt, newt;
            //            oldt.tv_nsec = 0;
            //            oldt.tv_sec = old->getAuxData(ONLINE_SERVICE_LAST_UPDATE).toLong();
            //            newt.tv_nsec = 0;
            //            newt.tv_sec = obj->getAuxData(ONLINE_SERVICE_LAST_UPDATE).toLong();
            content->updateObject(obj);
        }

//        if (server->getShutdownStatus())
//            return false;

    } while (obj != nullptr);

    return false;
}

#endif //ATRAILERS
