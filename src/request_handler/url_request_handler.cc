/*MT*

    MediaTomb - http://www.mediatomb.cc/

    url_request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file url_request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#ifdef HAVE_CURL
#include "url_request_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/transcoding.h"
#include "content/content.h"
#include "exceptions.h"
#include "iohandler/curl_io_handler.h"
#include "transcoding/transcode_dispatcher.h"
#include "upnp/compat.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"
#include "util/url.h"
#include "util/url_utils.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service_helper.h"
#endif

bool URLRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("start");

    auto params = URLUtils::parseParameters(filename, LINK_URL_REQUEST_HANDLER);
    auto obj = loadObject(params);
    if (!obj->isExternalItem()) {
        throw_std_runtime_error("getInfo: object is not an external url item");
    }

    auto item = std::static_pointer_cast<CdsItemExternalURL>(obj);

    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

    std::string mimeType;

    if (!trProfile.empty()) {
        auto tp = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)
                      ->getByName(trProfile);
        if (!tp)
            throw_std_runtime_error("Transcoding requested but no profile matching the name {} found", trProfile);

        mimeType = tp->getTargetMimeType();
        UpnpFileInfo_set_FileLength(info, -1);
    } else {
#ifdef ONLINE_SERVICES
        std::string url = item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) ? OnlineServiceHelper::resolveURL(item) : item->getLocation().string();
#else
        std::string url = item->getLocation().string();
#endif
        log_debug("Online content url: {}", url);
        try {
            auto st = URL(url).getInfo();
            UpnpFileInfo_set_FileLength(info, st.getSize());
            log_debug("URL used for request: {}", st.getURL());
        } catch (const std::runtime_error& ex) {
            log_warning("{}", ex.what());
            UpnpFileInfo_set_FileLength(info, -1);
        }

        mimeType = item->getMimeType();
    }

    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);
    GrbUpnpFileInfoSetContentType(info, mimeType);
    log_debug("web_get_info(): end");

    return quirks && quirks->getClient();
}

std::unique_ptr<IOHandler> URLRequestHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    log_debug("start");

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw_std_runtime_error("UPNP_WRITE unsupported");

    auto params = URLUtils::parseParameters(filename, LINK_URL_REQUEST_HANDLER);
    auto obj = loadObject(params);
    if (!obj->isExternalItem()) {
        throw_std_runtime_error("object is not an external url item");
    }

    auto item = std::static_pointer_cast<CdsItemExternalURL>(obj);
#ifdef ONLINE_SERVICES
    std::string url = item->getFlag(OBJECT_FLAG_ONLINE_SERVICE) ? OnlineServiceHelper::resolveURL(item) : item->getLocation().string();
#else
    std::string url = item->getLocation().string();
#endif
    log_debug("Online content url: {}", url);

    auto it = params.find(CLIENT_GROUP_TAG);
    std::string group = DEFAULT_CLIENT_GROUP;
    if (it != params.end()) {
        group = it->second;
    }

    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (!trProfile.empty()) {
        auto tp = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)
                      ->getByName(trProfile);
        if (!tp)
            throw_std_runtime_error("Transcoding of file {} but no profile matching the name {} found", url, trProfile);

        auto trD = std::make_unique<TranscodeDispatcher>(content);
        auto ioHandler = trD->serveContent(tp, url, item, group, "");

        log_debug("end");
        return ioHandler;
    }

    auto ioHandler = std::make_unique<CurlIOHandler>(config, url,
        config->getIntOption(ConfigVal::URL_REQUEST_CURL_BUFFER_SIZE),
        config->getIntOption(ConfigVal::URL_REQUEST_CURL_FILL_SIZE));
    content->triggerPlayHook(group, obj);
    return ioHandler;
}

#endif // HAVE_CURL
