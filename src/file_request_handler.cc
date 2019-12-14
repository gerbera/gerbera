/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_request_handler.cc - this file is part of MediaTomb.
    
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

/// \file file_request_handler.cc

#include <sys/stat.h>


#include "file_io_handler.h"
#include "file_request_handler.h"
#include "metadata_handler.h"
#include "play_hook.h"
#include "process.h"
#include "server.h"
#include "session_manager.h"
#include "update_manager.h"

#include "util/headers.h"

#include "transcoding/transcode_dispatcher.h"

using namespace zmm;
using namespace mxml;

FileRequestHandler::FileRequestHandler(UpnpXMLBuilder* xmlBuilder)
    : RequestHandler()
    , xmlBuilder(xmlBuilder) {};

void FileRequestHandler::getInfo(IN const char* filename, OUT UpnpFileInfo* info)
{
    Headers headers;
    log_debug("start\n");

    std::string mimeType;
    int objectID;
    std::string tr_profile;

    struct stat statbuf;
    int ret = 0;
    bool is_srt = false;

    std::string parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    Ref<Dictionary> dict(new Dictionary());
    dict->decodeSimple(parameters);

    log_debug("full url (filename): %s, parameters: %s\n", filename, parameters.c_str());

    std::string objID = dict->get(_("object_id"));
    if (objID.empty()) {
        //log_error("object_id not found in url\n");
        throw _Exception(_("getInfo: object_id not found"));
    }
    objectID = std::stoi(objID);

    //log_debug("got ObjectID: [%s]\n", object_id.c_str());

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType)) {
        throw _Exception(_("requested object is not an item"));
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    std::string path = item->getLocation();

    // determining which resource to serve
    int res_id = 0;
    std::string s_res_id = dict->get(_(URL_RESOURCE_ID));
    if (string_ok(s_res_id) && (s_res_id != _(URL_VALUE_TRANSCODE_NO_RES_ID)))
        res_id = std::stoi(s_res_id);
    else
        res_id = -1;

    std::string ext = dict->get(_("ext"));
    size_t edot = ext.rfind('.');
    if (edot != std::string::npos)
        ext = ext.substr(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi")
        || (ext == ".sub")) {
        size_t dot = path.rfind('.');
        if (dot != std::string::npos) {
            path = path.substr(0, dot);
        }

        path = path + ext;
        mimeType = _(MIMETYPE_TEXT);

        // reset resource id
        res_id = 0;
        is_srt = true;
    }

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt)
            throw SubtitlesNotFoundException(
                _("Subtitle file ") + path + " is not available.");
        else
            throw _Exception(
                _("Failed to open ") + path + " - " + strerror(errno));
    }

    if (access(path.c_str(), R_OK) == 0) {
        UpnpFileInfo_set_IsReadable(info, 1);
    } else {
        UpnpFileInfo_set_IsReadable(info, 0);
    }

    std::string header;
    log_debug("path: %s\n", path.c_str());
    size_t slash_pos = path.rfind(DIR_SEPARATOR);
    if (slash_pos != std::string::npos) {
        if (slash_pos < path.length() - 1) {
            slash_pos++;

            header = _("Content-Disposition: attachment; filename=\"")
                + path.substr(slash_pos) + _("\"");
        }
    }

    tr_profile = dict->get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));

    // for transcoded resourecs res_id will always be negative
    log_debug("fetching resource id %d\n", res_id);
    std::string rh = dict->get(_(RESOURCE_HANDLER));

    if (((res_id > 0) && (res_id < item->getResourceCount()))
        || ((res_id > 0) && string_ok(rh))) {

        int res_handler;
        if (string_ok(rh))
            res_handler = std::stoi(rh);
        else {
            Ref<CdsResource> resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            std::string protocolInfo = item->getResource(res_id)->getAttributes()->get(_("protocolInfo"));
            if (!protocolInfo.empty()) {
                mimeType = getMTFromProtocolInfo(protocolInfo);
            }
        }

        Ref<MetadataHandler> h = MetadataHandler::createHandler(res_handler);
        if (!string_ok(mimeType))
            mimeType = h->getMimeType();

        off_t size = -1;
        h->serveContent(item, res_id, &(size));
        UpnpFileInfo_set_FileLength(info, size);
    } else if (!is_srt && string_ok(tr_profile)) {

        Ref<TranscodingProfile> tp = ConfigManager::getInstance()
                                         ->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)
                                         ->getByName(tr_profile);

        if (tp == nullptr)
            throw _Exception(_("Transcoding of file ") + path
                + " but no profile matching the name "
                + tr_profile + " found");

        mimeType = tp->getTargetMimeType();

        Ref<Dictionary> mappings = ConfigManager::getInstance()
                                       ->getDictionaryOption(
                                           CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        if (mappings->get(mimeType) == CONTENT_TYPE_PCM) {
            std::string freq = item->getResource(0)
                              ->getAttribute(MetadataHandler::getResAttrName(
                                  R_SAMPLEFREQUENCY));
            std::string nrch = item->getResource(0)
                              ->getAttribute(MetadataHandler::getResAttrName(
                                  R_NRAUDIOCHANNELS));

            if (string_ok(freq))
                mimeType = mimeType + _(";rate=") + freq;
            if (string_ok(nrch))
                mimeType = mimeType + _(";channels=") + nrch;
        }

        UpnpFileInfo_set_FileLength(info, -1);
    } else {
        UpnpFileInfo_set_FileLength(info, statbuf.st_size);

        Ref<ConfigManager> cfg = ConfigManager::getInstance();
        if (cfg->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
            if (startswith_string(item->getMimeType(), "video")) {
                // Look for subtitle file and returns it's URL
                // in CaptionInfo.sec response header.
                // To be more compliant with original Samsung
                // server we should check for getCaptionInfo.sec: 1
                // request header.
                std::vector<std::string> subexts(4);
                subexts.push_back(_(".srt"));
                subexts.push_back(_(".ssa"));
                subexts.push_back(_(".smi"));
                subexts.push_back(_(".sub"));

                std::string bfilepath = path.substr(0, path.rfind('.'));
                std::string validext;
                for (size_t i = 0; i < subexts.size(); i++) {
                    std::string ext = subexts[i];

                    std::string fpath = bfilepath + ext;
                    if (access(fpath.c_str(), R_OK) == 0) {
                        validext = ext;
                        break;
                    }
                }

                if (validext.length() > 0) {
                    std::string burlpath = _(filename);
                    burlpath = burlpath.substr(0, burlpath.rfind('.'));
                    Ref<Server> server = Server::getInstance(); // FIXME server sigleton usage
                    std::string url = _("http://") + server->getIP() + ":" + server->getPort() + burlpath + validext;
                    headers.addHeader(_("CaptionInfo.sec:"), url);
                }
            }
        }
        Ref<Dictionary> mappings = cfg->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = getDLNAContentHeader(mappings->get(item->getMimeType()));
        if (string_ok(dlnaContentHeader)) {
            headers.addHeader(_(D_HTTP_CONTENT_FEATURES_HEADER), dlnaContentHeader);
        }
    }

    if (!string_ok(mimeType))
        mimeType = item->getMimeType();
    std::string dlnaTransferHeader = getDLNATransferHeader(mimeType);
    if (string_ok(dlnaTransferHeader)) {
        headers.addHeader(_(D_HTTP_TRANSFER_MODE_HEADER), dlnaTransferHeader);
    }

    //log_debug("sizeof off_t %d, statbuf.st_size %d\n", sizeof(off_t), sizeof(statbuf.st_size));
    //log_debug("getInfo: file_length: " OFF_T_SPRINTF "\n", statbuf.st_size);

    UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
    UpnpFileInfo_set_IsDirectory(info, S_ISDIR(statbuf.st_mode));
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimeType.c_str()));

    headers.writeHeaders(info);

    // log_debug("getInfo: Requested %s, ObjectID: %s, Location: %s\n, MimeType: %s\n",
    //      filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("web_get_info(): end\n");
}

Ref<IOHandler> FileRequestHandler::open(IN const char* filename,
    IN enum UpnpOpenFileMode mode, IN std::string range)
{
    log_debug("start\n");

    // We explicitly do not support UPNP_WRITE due to security reasons.
    if (mode != UPNP_READ) {
        throw _Exception(_("UPNP_WRITE unsupported"));
    }

    std::string parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    Dictionary params;
    params.decodeSimple(parameters);
    log_debug("full url (filename): %s, parameters: %s\n", filename, parameters.c_str());

    std::string objID = params.get(_("object_id"));
    if (objID.empty()) {
        throw _Exception(_("object_id not found in parameters"));
    }

    int objectID = std::stoi(objID);

    log_debug("Opening media file with object id %d\n", objectID);
    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType)) {
        throw _Exception(_("requested object is not an item"));
    }

    // determining which resource to serve
    int res_id = 0;
    std::string s_res_id = params.get(_(URL_RESOURCE_ID));
    if (string_ok(s_res_id) && (s_res_id != _(URL_VALUE_TRANSCODE_NO_RES_ID))) {
        res_id = std::stoi(s_res_id);
    } else {
        res_id = -1;
    }

    // update item info by running action
    if (IS_CDS_ACTIVE_ITEM(objectType) && (res_id == 0)) { // check - if thumbnails, then no action, just show
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        Ref<Element> inputElement = xmlBuilder->renderObject(obj, true);

        inputElement->setAttribute(_(XML_DC_NAMESPACE_ATTR), _(XML_DC_NAMESPACE));
        inputElement->setAttribute(_(XML_UPNP_NAMESPACE_ATTR), _(XML_UPNP_NAMESPACE));
        std::string action = aitem->getAction();
        std::string input = inputElement->print();
        std::string output;

        log_debug("Script input: %s\n", input.c_str());
        if (strncmp(action.c_str(), "http://", 7)) {
#ifdef TOMBDEBUG
            struct timespec before;
            getTimespecNow(&before);
#endif
            output = run_simple_process(action, _("run"), input);
#ifdef TOMBDEBUG
            long delta = getDeltaMillis(&before);
            log_debug("script executed in %ld milliseconds\n", delta);
#endif
        } else {
            /// \todo actually fetch...
            log_debug("fetching %s\n", action.c_str());
            output = input;
        }
        log_debug("Script output: %s\n", output.c_str());

        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        aitem->copyTo(clone);

        xmlBuilder->updateObject(clone, output);

        if (!aitem->equals(clone, true)) // check for all differences
        {
            Ref<UpdateManager> um = UpdateManager::getInstance();
            Ref<SessionManager> sm = SessionManager::getInstance();

            log_debug("Item changed, updating database\n");
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            um->containerChanged(containerChanged);
            sm->containerChangedUI(containerChanged);

            if (!aitem->equals(clone)) // check for visible differences
            {
                log_debug("Item changed visually, updating parent\n");
                um->containerChanged(clone->getParentID(), FLUSH_ASAP);
            }
            obj = clone;
        } else {
            log_debug("Item untouched...\n");
        }
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    std::string path = item->getLocation();
    bool is_srt = false;

    std::string mimeType;
    std::string ext = params.get(_("ext"));
    size_t edot = ext.rfind('.');
    if (edot != std::string::npos)
        ext = ext.substr(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi") || (ext == ".sub")) {
        size_t dot = path.rfind('.');
        if (dot != std::string::npos) {
            path = path.substr(0, dot);
        }

        path = path + ext;
        mimeType = _(MIMETYPE_TEXT);
        // reset resource id
        res_id = 0;
        is_srt = true;
    }

    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt)
            throw SubtitlesNotFoundException(_("Subtitle file ") + path + " is not available.");
        else
            throw _Exception(_("Failed to open ") + path + " - " + strerror(errno));
    }

    log_debug("fetching resource id %d\n", res_id);

    std::string tr_profile = params.get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));
    if (string_ok(tr_profile)) {
        if (res_id != (-1)) {
            throw _Exception(_("Invalid resource ID given!"));
        }
    } else {
        if (res_id == -1) {
            throw _Exception(_("Invalid resource ID given!"));
        }
    }

    // some resources are created dynamically and not saved in the database,
    // so we can not load such a resource for a particular item, we will have
    // to trust the resource handler parameter
    std::string rh = params.get(_(RESOURCE_HANDLER));
    if (((res_id > 0) && (res_id < item->getResourceCount())) || ((res_id > 0) && string_ok(rh))) {
        //info->file_length = -1;

        int res_handler;
        if (string_ok(rh))
            res_handler = std::stoi(rh);
        else {
            Ref<CdsResource> resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            std::string protocolInfo = item->getResource(res_id)->getAttributes()->get(_("protocolInfo"));
            if (!protocolInfo.empty()) {
                mimeType = getMTFromProtocolInfo(protocolInfo);
            }
        }

        Ref<MetadataHandler> h = MetadataHandler::createHandler(res_handler);

        if (!string_ok(mimeType))
            mimeType = h->getMimeType();

        /* FIXME Upstream upnp / DNLA
#ifdef EXTEND_PROTOCOLINFO
        header = getDLNAtransferHeader(mimeType, header);
#endif

        if (string_ok(header))
                info->http_header = ixmlCloneDOMString(header.c_str());
        */

        //info->content_type = ixmlCloneDOMString(mimeType.c_str());
        //Ref<IOHandler> io_handler = h->serveContent(item, res_id, &(info->file_length));

        off_t filelength = -1;
        Ref<IOHandler> io_handler = h->serveContent(item, res_id, &filelength);
        io_handler->open(mode);
        log_debug("end\n");
        return io_handler;

    } else {
        if (!is_srt && string_ok(tr_profile)) {
            std::string range = params.get(_("range"));

            Ref<TranscodeDispatcher> tr_d(new TranscodeDispatcher());
            Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(tr_profile);
            return tr_d->open(tp, path, RefCast(item, CdsObject), range);
        } else {
            if (mimeType.empty())
                mimeType = item->getMimeType();

            /* FIXME Upstream headers / DNLA
            info->file_length = statbuf.st_size;
            info->content_type = ixmlCloneDOMString(mimeType.c_str());

            log_debug("Adding content disposition header: %s\n", 
                    header.c_str());
            // if we are dealing with a regular file we should add the
            // Accept-Ranges: bytes header, in order to indicate that we support
            // seeking
            if (S_ISREG(statbuf.st_mode))
            {
                if (string_ok(header))
                    header = header + _("\r\n");

                header = header + _("Accept-Ranges: bytes");
            }

            header = getDLNAtransferHeader(mimeType, header);

            if (string_ok(header))
                info->http_header = ixmlCloneDOMString(header.c_str());
            */

            Ref<IOHandler> io_handler(new FileIOHandler(path));
            io_handler->open(mode);
            PlayHook::getInstance()->trigger(obj);
            log_debug("end\n");
            return io_handler;
        }
    }
}
