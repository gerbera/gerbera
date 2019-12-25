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

#include <ixml.h>

#include "common.h"
#include "server.h"
#include "storage.h"

#include "buffered_io_handler.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "play_hook.h"
#include "url_request_handler.h"

#ifdef ONLINE_SERVICES
#include "online_service_helper.h"
#endif
#include "curl_io_handler.h"
#include "transcoding/transcode_dispatcher.h"
#include "url.h"

using namespace zmm;
using namespace mxml;

URLRequestHandler::URLRequestHandler()
    : RequestHandler()
{
}

void URLRequestHandler::getInfo(const char *filename, UpnpFileInfo *info)
{
    log_debug("start\n");

    std::string header;
    std::string mimeType;
    int objectID;
    std::string tr_profile;

    std::string url, parameters;
    parameters = (filename + strlen(LINK_URL_REQUEST_HANDLER));

    Ref<Dictionary> dict(new Dictionary());
    dict->decodeSimple(parameters);

    log_debug("full url (filename): %s, parameters: %s\n",
        filename, parameters.c_str());

    std::string objID = dict->get("object_id");
    if (objID.empty()) {
        //log_error("object_id not found in url\n");
        throw _Exception("getInfo: object_id not found");
    } else
        objectID = std::stoi(objID);

    //log_debug("got ObjectID: [%s]\n", object_id.c_str());

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        throw _Exception("getInfo: object is not an external url item");
    }

    tr_profile = dict->get(URL_PARAM_TRANSCODE_PROFILE_NAME);

    if (string_ok(tr_profile)) {
        Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(tr_profile);

        if (tp == nullptr)
            throw _Exception("Transcoding requested but no profile "
                             "matching the name "
                + tr_profile + " found");

        mimeType = tp->getTargetMimeType();
        UpnpFileInfo_set_FileLength(info, -1);
    } else {
        Ref<CdsItemExternalURL> item = RefCast(obj, CdsItemExternalURL);

#ifdef ONLINE_SERVICES
        if (item->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
            /// \todo write a helper class that will handle various online
            /// services
            Ref<OnlineServiceHelper> helper(new OnlineServiceHelper());
            url = helper->resolveURL(item);
        } else
#endif
        {
            url = item->getLocation();
        }

        log_debug("Online content url: %s\n", url.c_str());
        Ref<URL> u(new URL());
        Ref<URL::Stat> st;
        try {
            st = u->getInfo(url);
            UpnpFileInfo_set_FileLength(info, st->getSize());
            header = "Accept-Ranges: bytes";
            log_debug("URL used for request: %s\n", st->getURL().c_str());
        } catch (const Exception& ex) {
            log_warning("%s\n", ex.getMessage().c_str());
            UpnpFileInfo_set_FileLength(info, -1);
        }

        mimeType = item->getMimeType();
    }

    UpnpFileInfo_set_IsReadable(info, 1);
    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);

    // FIX EXTRA HEADERS
//    if (string_ok(header)) {
//        UpnpFileInfo_set_ExtraHeaders(info,
//            ixmlCloneDOMString(header.c_str()));
//    }

    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimeType.c_str()));
    log_debug("web_get_info(): end\n");

    /// \todo transcoding for get_info
}

Ref<IOHandler> URLRequestHandler::open(const char* filename,
    enum UpnpOpenFileMode mode,
    std::string range)
{
    int objectID;
    std::string mimeType;
    std::string header;
    std::string tr_profile;

    log_debug("start\n");

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw _Exception("UPNP_WRITE unsupported");

    std::string url, parameters;
    parameters = (filename + strlen(LINK_URL_REQUEST_HANDLER));

    Ref<Dictionary> dict(new Dictionary());
    dict->decodeSimple(parameters);
    log_debug("full url (filename): %s, parameters: %s\n",
        filename, parameters.c_str());

    std::string objID = dict->get("object_id");
    if (objID.empty()) {
        throw _Exception("object_id not found");
    } else
        objectID = std::stoi(objID);

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM_EXTERNAL_URL(objectType)) {
        throw _Exception("object is not an external url item");
    }

    Ref<CdsItemExternalURL> item = RefCast(obj, CdsItemExternalURL);

#ifdef ONLINE_SERVICES
    if (item->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        Ref<OnlineServiceHelper> helper(new OnlineServiceHelper());
        url = helper->resolveURL(item);
    } else
#endif
    {
        url = item->getLocation();
    }

    log_debug("Online content url: %s\n", url.c_str());

    //info->is_readable = 1;
    //info->last_modified = 0;
    //info->is_directory = 0;
    //info->http_header = NULL;

    tr_profile = dict->get(URL_PARAM_TRANSCODE_PROFILE_NAME);

    if (string_ok(tr_profile)) {
        Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(tr_profile);

        if (tp == nullptr)
            throw _Exception("Transcoding of file " + url + " but no profile matching the name " + tr_profile + " found");

        Ref<TranscodeDispatcher> tr_d(new TranscodeDispatcher());
        return tr_d->open(tp, url, RefCast(item, CdsObject), range);
    } else {
        Ref<URL> u(new URL());
        Ref<URL::Stat> st;
        try {
            st = u->getInfo(url);
            // info->file_length = st->getSize();
            header = "Accept-Ranges: bytes";
            log_debug("URL used for request: %s\n", st->getURL().c_str());
        } catch (const Exception& ex) {
            log_warning("%s\n", ex.getMessage().c_str());
            //info->file_length = -1;
        }
        mimeType = item->getMimeType();
        // info->content_type = ixmlCloneDOMString(mimeType.c_str());
    }

    /* FIXME headers
    if (string_ok(header))
        info->http_header = ixmlCloneDOMString(header.c_str());
    */

    ///\todo make curl io handler configurable for url request handler
    Ref<IOHandler> io_handler(new CurlIOHandler(url, nullptr, 1024 * 1024, 0));

    io_handler->open(mode);

    PlayHook::getInstance()->trigger(obj);
    return io_handler;
}

#endif // HAVE_CURL
