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

#include "handler/headers.h"
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

    String mimeType;
    int objectID;
    String tr_profile;

    struct stat statbuf;
    int ret = 0;
    bool is_srt = false;

    String parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    Ref<Dictionary> dict(new Dictionary());
    dict->decodeSimple(parameters);

    log_debug("full url (filename): %s, parameters: %s\n", filename, parameters.c_str());

    String objID = dict->get(_("object_id"));
    if (objID == nullptr) {
        //log_error("object_id not found in url\n");
        throw _Exception(_("getInfo: object_id not found"));
    }
    objectID = objID.toInt();

    //log_debug("got ObjectID: [%s]\n", object_id.c_str());

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType)) {
        throw _Exception(_("requested object is not an item"));
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    String path = item->getLocation();

    // determining which resource to serve
    int res_id = 0;
    String s_res_id = dict->get(_(URL_RESOURCE_ID));
    if (string_ok(s_res_id) && (s_res_id != _(URL_VALUE_TRANSCODE_NO_RES_ID)))
        res_id = s_res_id.toInt();
    else
        res_id = -1;

    String ext = dict->get(_("ext"));
    int edot = ext.rindex('.');
    if (edot > -1)
        ext = ext.substring(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi")
        || (ext == ".sub")) {
        int dot = path.rindex('.');
        if (dot > -1) {
            path = path.substring(0, dot);
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

    String header;
    log_debug("path: %s\n", path.c_str());
    int slash_pos = path.rindex(DIR_SEPARATOR);
    if (slash_pos >= 0) {
        if (slash_pos < path.length() - 1) {
            slash_pos++;

            header = _("Content-Disposition: attachment; filename=\"")
                + path.substring(slash_pos) + _("\"");
        }
    }

    tr_profile = dict->get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));

    // for transcoded resourecs res_id will always be negative
    log_debug("fetching resource id %d\n", res_id);
    String rh = dict->get(_(RESOURCE_HANDLER));

    if (((res_id > 0) && (res_id < item->getResourceCount()))
        || ((res_id > 0) && string_ok(rh))) {

        int res_handler;
        if (string_ok(rh))
            res_handler = rh.toInt();
        else {
            Ref<CdsResource> resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            String protocolInfo = item->getResource(res_id)->getAttributes()->get(_("protocolInfo"));
            if (protocolInfo != nullptr) {
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
            String freq = item->getResource(0)
                              ->getAttribute(MetadataHandler::getResAttrName(
                                  R_SAMPLEFREQUENCY));
            String nrch = item->getResource(0)
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
            if (item->getMimeType().startsWith(_("video"))) {
                // Look for subtitle file and returns it's URL
                // in CaptionInfo.sec response header.
                // To be more compliant with original Samsung
                // server we should check for getCaptionInfo.sec: 1
                // request header.
                Ref<Array<StringBase>> subexts(new Array<StringBase>());
                subexts->append(_(".srt"));
                subexts->append(_(".ssa"));
                subexts->append(_(".smi"));
                subexts->append(_(".sub"));

                String bfilepath = path.substring(0, path.rindex('.'));
                String validext;
                for (int i = 0; i < subexts->size(); i++) {
                    String ext = subexts->get(i);

                    String fpath = bfilepath + ext;
                    if (access(fpath.c_str(), R_OK) == 0) {
                        validext = ext;
                        break;
                    }
                }

                if (validext.length() > 0) {
                    String burlpath = _(filename);
                    burlpath = burlpath.substring(0, burlpath.rindex('.'));
                    Ref<Server> server = Server::getInstance(); // FIXME server sigleton usage
                    String url = _("http://") + server->getIP() + ":" + server->getPort() + burlpath + validext;
                    headers.addHeader(_("CaptionInfo.sec: ") + url);
                }
            }
        }
        Ref<Dictionary> mappings = cfg->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        headers.addHeader(getDLNAcontentHeader(mappings->get(item->getMimeType())));
    }

    if (!string_ok(mimeType))
        mimeType = item->getMimeType();
    headers.addHeader(getDLNAtransferHeader(mimeType));

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
    IN enum UpnpOpenFileMode mode, IN zmm::String range)
{
    log_debug("start\n");

    // We explicitly do not support UPNP_WRITE due to security reasons.
    if (mode != UPNP_READ) {
        throw _Exception(_("UPNP_WRITE unsupported"));
    }

    String parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    Dictionary params;
    params.decodeSimple(parameters);
    log_debug("full url (filename): %s, parameters: %s\n", filename, parameters.c_str());

    String objID = params.get(_("object_id"));
    if (objID == nullptr) {
        throw _Exception(_("object_id not found in parameters"));
    }

    int objectID = objID.toInt();

    log_debug("Opening media file with object id %d\n", objectID);
    Ref<Storage> storage = Storage::getInstance();
    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType)) {
        throw _Exception(_("requested object is not an item"));
    }

    // determining which resource to serve
    int res_id = 0;
    String s_res_id = params.get(_(URL_RESOURCE_ID));
    if (string_ok(s_res_id) && (s_res_id != _(URL_VALUE_TRANSCODE_NO_RES_ID))) {
        res_id = s_res_id.toInt();
    } else {
        res_id = -1;
    }

    // update item info by running action
    if (IS_CDS_ACTIVE_ITEM(objectType) && (res_id == 0)) { // check - if thumbnails, then no action, just show
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        Ref<Element> inputElement = xmlBuilder->renderObject(obj, true);

        inputElement->setAttribute(_(XML_DC_NAMESPACE_ATTR), _(XML_DC_NAMESPACE));
        inputElement->setAttribute(_(XML_UPNP_NAMESPACE_ATTR), _(XML_UPNP_NAMESPACE));
        String action = aitem->getAction();
        String input = inputElement->print();
        String output;

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

    String path = item->getLocation();
    bool is_srt = false;

    String mimeType;
    String ext = params.get(_("ext"));
    int edot = ext.rindex('.');
    if (edot > -1)
        ext = ext.substring(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi") || (ext == ".sub")) {
        int dot = path.rindex('.');
        if (dot > -1) {
            path = path.substring(0, dot);
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

    String tr_profile = params.get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));
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
    String rh = params.get(_(RESOURCE_HANDLER));
    if (((res_id > 0) && (res_id < item->getResourceCount())) || ((res_id > 0) && string_ok(rh))) {
        //info->file_length = -1;

        int res_handler;
        if (string_ok(rh))
            res_handler = rh.toInt();
        else {
            Ref<CdsResource> resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            String protocolInfo = item->getResource(res_id)->getAttributes()->get(_("protocolInfo"));
            if (protocolInfo != nullptr) {
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
            String range = params.get(_("range"));

            Ref<TranscodeDispatcher> tr_d(new TranscodeDispatcher());
            Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(tr_profile);
            return tr_d->open(tp, path, RefCast(item, CdsObject), range);
        } else {
            if (mimeType == nullptr)
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
