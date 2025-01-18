/*GRB*

    Gerbera - https://gerbera.io/

    curl_online_service.cc - this file is part of Gerbera.

    Copyright (C) 2021-2025 Gerbera Contributors

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
#ifdef ONLINE_SERVICES
#define GRB_LOG_FAC GrbLogFacility::online
#include "curl_online_service.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "content/layout/layout.h"
#include "context.h"
#include "database/database.h"
#include "exceptions.h"
#include "util/string_converter.h"
#include "util/tools.h"
#include "util/url.h"

CurlContentHandler::CurlContentHandler(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
    , database(context->getDatabase())
{
}

CurlOnlineService::CurlOnlineService(const std::shared_ptr<Content>& content, std::string serviceName)
    : OnlineService(content)
    , curl_handle(curl_easy_init())
    , serviceName(std::move(serviceName))
    , converterManager(content->getContext()->getConverterManager())
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
    auto sc = converterManager->i2i();

    std::string buffer;

    try {
        log_debug("DOWNLOADING URL: {}", service_url);
#ifdef GRBDEBUG
        bool verbose = GrbLogger::Logger.isDebugging(GRB_LOG_FAC) || GrbLogger::Logger.isDebugLogging();
#else
        bool verbose = false;
#endif
        buffer = URL(service_url, curl_handle).download(&retcode, false, verbose, true).second;
    } catch (const std::runtime_error& ex) {
        log_error("Failed to download {} XML data: {}", serviceName, ex.what());
        return nullptr;
    }

    if (buffer.empty())
        return nullptr;

    if (retcode != 200)
        return nullptr;

    log_debug("GOT BUFFER {}", buffer);
    auto doc = std::make_unique<pugi::xml_document>();
    auto [mval, err] = sc->convert(buffer);
    if (!err.empty()) {
        log_warning("{}: {}", service_url, err);
    }
    pugi::xml_parse_result result = doc->load_string(mval.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        log_error("Error parsing {} XML: {}", serviceName, result.description());
        return nullptr;
    }

    return doc;
}

bool CurlOnlineService::refreshServiceData(const std::shared_ptr<Layout>& layout)
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
    if (!reply) {
        log_debug("Failed to get XML content from {} service", serviceName);
        throw_std_runtime_error("Failed to get XML content from {} service", serviceName);
    }

    auto sc = getContentHandler();
    sc->setServiceContent(std::move(reply));

    auto mappings = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    std::shared_ptr<CdsObject> obj;
    do {
        obj = sc->getNextObject();
        if (!obj)
            continue;

        obj->setVirtual(true);

        auto item = std::static_pointer_cast<CdsItem>(obj);
        auto old = database->loadObjectByServiceID(item->getServiceID(), DEFAULT_CLIENT_GROUP);
        if (!old) {
            log_debug("Adding new {} object", serviceName);

            if (layout) {
                std::string mimetype = item->getMimeType();
                std::string contentType = getValueOrDefault(mappings, mimetype);
                std::vector<int> refObjects;

                layout->processCdsObject(obj, nullptr, "",
                    contentType,
                    AutoscanDirectory::ContainerTypesDefaults,
                    refObjects);
            }
        } else {
            log_debug("Updating existing {} object", serviceName);
            obj->setID(old->getID());
            obj->setParentID(old->getParentID());
            content->updateObject(obj);
        }
    } while (obj || !sc->isEnd());

    return false;
}

#endif // ONLINE_SERVICES
#endif // HAVE_CURL
