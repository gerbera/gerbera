/*GRB*

    Gerbera - https://gerbera.io/

    curl_online_service.cc - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file curl_online_service.cc

#ifdef HAVE_CURL
#include "curl_online_service.h" // API

#include <string>

#include "config/config_manager.h"
#include "config/config_options.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "util/string_converter.h"
#include "util/url.h"

CurlContentHandler::CurlContentHandler(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
    , database(context->getDatabase())
{
}

CurlOnlineService::CurlOnlineService(std::shared_ptr<ContentManager> content, std::string serviceName)
    : OnlineService(std::move(content))
    , curl_handle(curl_easy_init())
    , pid(0)
    , serviceName(std::move(serviceName))
{
    if (!curl_handle)
        throw_std_runtime_error("failed to initialize curl");
}

CurlOnlineService::~CurlOnlineService()
{
    if (curl_handle)
        curl_easy_cleanup(curl_handle);
}

std::string CurlOnlineService::getServiceName() const
{
    return serviceName;
}

std::unique_ptr<pugi::xml_document> CurlOnlineService::getData()
{
    long retcode;
    auto sc = StringConverter::i2i(config);

    std::string buffer;

    try {
        log_debug("DOWNLOADING URL: {}", service_url.c_str());
        buffer = URL::download(service_url, &retcode,
            curl_handle, false, true, true);
    } catch (const std::runtime_error& ex) {
        log_error("Failed to download {} XML data: {}", serviceName, ex.what());
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
        log_error("Error parsing {} XML: {}", serviceName, result.description());
        return nullptr;
    }

    return doc;
}

bool CurlOnlineService::refreshServiceData(std::shared_ptr<Layout> layout)
{
    log_debug("Refreshing {}", serviceName);
    // the layout is in full control of the service items

    // this is a safeguard to ensure that this class is not called from
    // multiple threads - it is not allowed to use the same curl handle
    // from multiple threads
    // we do it here because the handle is initialized in a different thread
    // which is OK
    if (pid == 0)
        pid = pthread_self();

    if (pid != pthread_self())
        throw_std_runtime_error("Not allowed to call refreshServiceData from different threads");

    auto reply = getData();
    if (reply == nullptr) {
        log_debug("Failed to get XML content from {} service", serviceName);
        throw_std_runtime_error("Failed to get XML content from {} service", serviceName);
    }

    auto sc = getContentHandler();
    sc->setServiceContent(std::move(reply));

    std::shared_ptr<CdsObject> obj;
    do {
        /// \todo add try/catch here and a possibility do find out if we
        /// may request more stuff or if we are at the end of the list
        obj = sc->getNextObject();
        if (obj == nullptr)
            break;

        obj->setVirtual(true);

        auto old = database->loadObjectByServiceID(std::static_pointer_cast<CdsItem>(obj)->getServiceID());
        if (old == nullptr) {
            log_debug("Adding new {} object", serviceName);

            if (layout != nullptr)
                layout->processCdsObject(obj, "");
        } else {
            log_debug("Updating existing {} object", serviceName);
            obj->setID(old->getID());
            obj->setParentID(old->getParentID());
            content->updateObject(obj);
        }

        //        if (server->getShutdownStatus())
        //            return false;

    } while (obj != nullptr);

    return false;
}

#endif //HAVE_CURL
