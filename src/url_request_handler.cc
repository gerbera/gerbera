/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    url_request_handler.cc - this file is part of MediaTomb.
    
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

/// \file url_request_handler.cc

#ifdef HAVE_CURL
#include "url_request_handler.h" // API

#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"
#include "database/database.h"
#include "server.h"

#include "cds_objects.h"
#include "iohandler/buffered_io_handler.h"

#ifdef ONLINE_SERVICES
#include "onlineservice/online_service_helper.h"
#endif
#include "iohandler/curl_io_handler.h"
#include "transcoding/transcode_dispatcher.h"
#include "url.h"

URLRequestHandler::URLRequestHandler(std::shared_ptr<ContentManager> content)
    : RequestHandler(std::move(content))
{
}

void URLRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("start");

    auto params = parseParameters(filename, LINK_URL_REQUEST_HANDLER);
    auto obj = getObjectById(params);
    if (!obj->isExternalItem()) {
        throw_std_runtime_error("getInfo: object is not an external url item");
    }

    auto item = std::static_pointer_cast<CdsItemExternalURL>(obj);

    std::string tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

    std::string header;
    std::string mimeType;

    if (!tr_profile.empty()) {
        auto tp = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)
                      ->getByName(tr_profile);
        if (tp == nullptr)
            throw_std_runtime_error("Transcoding requested but no profile matching the name "
                + tr_profile + " found");

        mimeType = tp->getTargetMimeType();
        UpnpFileInfo_set_FileLength(info, -1);
    } else {
        std::string url;

#ifdef ONLINE_SERVICES
        if (item->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            /// \todo write a helper class that will handle various online
            /// services
            auto helper = std::make_unique<OnlineServiceHelper>();
            url = helper->resolveURL(item);
        } else
#endif
        {
            url = item->getLocation();
        }

        log_debug("Online content url: {}", url.c_str());
        try {
            auto st = URL::getInfo(url);
            UpnpFileInfo_set_FileLength(info, st->getSize());
            header = "Accept-Ranges: bytes";
            log_debug("URL used for request: {}", st->getURL().c_str());
        } catch (const std::runtime_error& ex) {
            log_warning("{}", ex.what());
            UpnpFileInfo_set_FileLength(info, -1);
        }

        mimeType = item->getMimeType();
    }

    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);

    // FIX EXTRA HEADERS
    //    if (!header.empty()) {
    //        UpnpFileInfo_set_ExtraHeaders(info,
    //            ixmlCloneDOMString(header.c_str()));
    //    }

#if defined(USING_NPUPNP)
    UpnpFileInfo_set_ContentType(info, mimeType);
#else
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimeType.c_str()));
#endif
    log_debug("web_get_info(): end");

    /// \todo transcoding for get_info
}

std::unique_ptr<IOHandler> URLRequestHandler::open(const char* filename, enum UpnpOpenFileMode mode)
{
    log_debug("start");

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw_std_runtime_error("UPNP_WRITE unsupported");

    auto params = parseParameters(filename, LINK_URL_REQUEST_HANDLER);
    auto obj = getObjectById(params);
    if (!obj->isExternalItem()) {
        throw_std_runtime_error("object is not an external url item");
    }

    auto item = std::static_pointer_cast<CdsItemExternalURL>(obj);

    std::string url;
#ifdef ONLINE_SERVICES
    if (item->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        auto helper = std::make_unique<OnlineServiceHelper>();
        url = helper->resolveURL(item);
    } else
#endif
    {
        url = item->getLocation();
    }

    log_debug("Online content url: {}", url.c_str());

    std::string tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (!tr_profile.empty()) {
        auto tp = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)
                      ->getByName(tr_profile);
        if (tp == nullptr)
            throw_std_runtime_error("Transcoding of file " + url + " but no profile matching the name " + tr_profile + " found");

        auto tr_d = std::make_unique<TranscodeDispatcher>(config, content);
        auto io_handler = tr_d->serveContent(tp, url, item, "");
        io_handler->open(mode);

        log_debug("end");
        return io_handler;
    }

    ///\todo make curl io handler configurable for url request handler
    auto io_handler = std::make_unique<CurlIOHandler>(url, nullptr, 1024 * 1024, 0);
    io_handler->open(mode);
    content->triggerPlayHook(obj);
    return io_handler;
}

#endif // HAVE_CURL
