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

#include "file_request_handler.h" // API

#include <filesystem>
#include <sys/stat.h>
#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "storage/storage.h"
#include "update_manager.h"
#include "util/process.h"
#include "web/session_manager.h"

#include "util/tools.h"
#include "util/upnp_headers.h"

#include "transcoding/transcode_dispatcher.h"

FileRequestHandler::FileRequestHandler(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content,
    std::shared_ptr<UpdateManager> updateManager, std::shared_ptr<web::SessionManager> sessionManager,
    UpnpXMLBuilder* xmlBuilder)
    : RequestHandler(std::move(config), std::move(storage))
    , content(std::move(content))
    , updateManager(std::move(updateManager))
    , sessionManager(std::move(sessionManager))
    , xmlBuilder(xmlBuilder)
{
}

void FileRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    Headers headers;
    log_debug("start");

    std::string tr_profile;
    int ret = 0;

    std::string parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    std::map<std::string, std::string> params;
    dict_decode_simple(parameters, &params);

    log_debug("full url (filename): {}, parameters: {}", filename, parameters.c_str());

    auto objIdIt = params.find("object_id");
    if (objIdIt == params.end()) {
        //log_error("object_id not found in url");
        throw std::runtime_error("getInfo: object_id not found");
    }
    int objectID = std::stoi(objIdIt->second);
    //log_debug("got ObjectID: {}", objectID);

    auto obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();
    if (!IS_CDS_ITEM(objectType)) {
        throw std::runtime_error("requested object is not an item");
    }
    auto item = std::static_pointer_cast<CdsItem>(obj);

    // determining which resource to serve
    int res_id = 0;
    auto res_id_it = params.find(URL_RESOURCE_ID);
    if (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID)
        res_id = std::stoi(res_id_it->second);
    else
        res_id = -1;

    fs::path path = item->getLocation();
    bool is_srt = false;

    std::string mimeType;
    std::string ext = getValueOrDefault(params, "ext");
    size_t edot = ext.rfind('.');
    if (edot != std::string::npos)
        ext = ext.substr(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi") || (ext == ".sub")) {
        // remove .ext
        std::string pathNoExt = path.parent_path() / path.stem();
        path = pathNoExt + ext;
        mimeType = MIMETYPE_TEXT;

        // reset resource id
        res_id = 0;
        is_srt = true;
    }

    struct stat statbuf;
    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt)
            throw SubtitlesNotFoundException("Subtitle file " + path.string() + " is not available.");

        throw std::runtime_error("Failed to open " + path.string() + " - " + strerror(errno));
    }

    if (access(path.c_str(), R_OK) == 0) {
        UpnpFileInfo_set_IsReadable(info, 1);
    } else {
        UpnpFileInfo_set_IsReadable(info, 0);
    }

    std::string header;
    log_debug("path: {}", path.c_str());
    if (!path.filename().empty()) {
        header = "Content-Disposition: attachment; filename=\"" + path.filename().string() + "\"";
    }

    // for transcoded resourecs res_id will always be negative
    tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

    log_debug("fetching resource id {}", res_id);

    // some resources are created dynamically and not saved in the database,
    // so we can not load such a resource for a particular item, we will have
    // to trust the resource handler parameter
    std::string rh = getValueOrDefault(params, RESOURCE_HANDLER);
    if (res_id > 0 && (res_id < item->getResourceCount() || !rh.empty())) {
        int res_handler;
        if (!rh.empty())
            res_handler = std::stoi(rh);
        else {
            auto resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            std::string protocolInfo = getValueOrDefault(item->getResource(res_id)->getAttributes(), "protocolInfo");
            if (!protocolInfo.empty()) {
                mimeType = getMTFromProtocolInfo(protocolInfo);
            }
        }

        auto h = MetadataHandler::createHandler(config, res_handler);
        if (mimeType.empty())
            mimeType = h->getMimeType();

        auto io_handler = h->serveContent(item, res_id);

        // get size
        io_handler->open(UPNP_READ);
        io_handler->seek(0L, SEEK_END);
        off_t size = io_handler->tell();
        io_handler->close();

        UpnpFileInfo_set_FileLength(info, size);
    } else if (!is_srt && !tr_profile.empty()) {
        auto tp = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)
                      ->getByName(tr_profile);
        if (tp == nullptr)
            throw std::runtime_error("Transcoding of file " + path.string()
                + " but no profile matching the name "
                + tr_profile + " found");

        mimeType = tp->getTargetMimeType();

        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        if (getValueOrDefault(mappings, mimeType) == CONTENT_TYPE_PCM) {
            std::string freq = item->getResource(0)
                                   ->getAttribute(MetadataHandler::getResAttrName(
                                       R_SAMPLEFREQUENCY));
            std::string nrch = item->getResource(0)
                                   ->getAttribute(MetadataHandler::getResAttrName(
                                       R_NRAUDIOCHANNELS));
            if (!freq.empty())
                mimeType = mimeType + ";rate=" + freq;
            if (!nrch.empty())
                mimeType = mimeType + ";channels=" + nrch;
        }

        UpnpFileInfo_set_FileLength(info, -1);
    } else {
        UpnpFileInfo_set_FileLength(info, statbuf.st_size);

        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK)) {
            if (startswith(item->getMimeType(), "video")) {
                // Look for subtitle file and returns it's URL
                // in CaptionInfo.sec response header.
                // To be more compliant with original Samsung
                // server we should check for getCaptionInfo.sec: 1
                // request header.
                std::vector<std::string> subexts;
                subexts.reserve(4);
                subexts.emplace_back(".srt");
                subexts.emplace_back(".ssa");
                subexts.emplace_back(".smi");
                subexts.emplace_back(".sub");

                // remove .ext
                std::string pathNoExt = path.parent_path() / path.stem();

                std::string validext;
                for (const auto& ext : subexts) {
                    std::string fpath = pathNoExt + ext;
                    if (access(fpath.c_str(), R_OK) == 0) {
                        validext = ext;
                        break;
                    }
                }

                if (validext.length() > 0) {
                    std::string burlpath = filename;
                    burlpath = burlpath.substr(0, burlpath.rfind('.'));
                    std::string url = "http://" + Server::getIP() + ":" + Server::getPort() + burlpath + validext;
                    headers.addHeader("CaptionInfo.sec:", url);
                }
            }
        }
        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = getDLNAContentHeader(config, getValueOrDefault(mappings, item->getMimeType()));
        if (!dlnaContentHeader.empty()) {
            headers.addHeader(D_HTTP_CONTENT_FEATURES_HEADER, dlnaContentHeader);
        }
    }

    if (mimeType.empty())
        mimeType = item->getMimeType();

    std::string dlnaTransferHeader = getDLNATransferHeader(config, mimeType);
    if (!dlnaTransferHeader.empty()) {
        headers.addHeader(D_HTTP_TRANSFER_MODE_HEADER, dlnaTransferHeader);
    }

    //log_debug("sizeof off_t {}, statbuf.st_size {}", sizeof(off_t), sizeof(statbuf.st_size));
    //log_debug("getInfo: file_length: " OFF_T_SPRINTF "", statbuf.st_size);

    UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
    UpnpFileInfo_set_IsDirectory(info, S_ISDIR(statbuf.st_mode));
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimeType.c_str()));

    headers.writeHeaders(info);

    // log_debug("getInfo: Requested {}, ObjectID: {}, Location: {}, MimeType: {}",
    //      filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("web_get_info(): end");
}

std::unique_ptr<IOHandler> FileRequestHandler::open(const char* filename,
    enum UpnpOpenFileMode mode, const std::string& range)
{
    log_debug("start");

    // We explicitly do not support UPNP_WRITE due to security reasons.
    if (mode != UPNP_READ) {
        throw std::runtime_error("UPNP_WRITE unsupported");
    }

    std::string parameters = (filename + strlen(LINK_FILE_REQUEST_HANDLER));

    std::map<std::string, std::string> params;
    dict_decode_simple(parameters, &params);
    log_debug("full url (filename): {}, parameters: {}", filename, parameters.c_str());

    auto objIdIt = params.find("object_id");
    if (objIdIt == params.end()) {
        throw std::runtime_error("object_id not found in parameters");
    }
    int objectID = std::stoi(objIdIt->second);

    log_debug("Opening media file with object id {}", objectID);
    auto obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();
    if (!IS_CDS_ITEM(objectType)) {
        throw std::runtime_error("requested object is not an item");
    }

    // determining which resource to serve
    int res_id = 0;
    auto res_id_it = params.find(URL_RESOURCE_ID);
    if (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID)
        res_id = std::stoi(res_id_it->second);
    else
        res_id = -1;

    // update item info by running action
    if (IS_CDS_ACTIVE_ITEM(objectType) && (res_id == 0)) { // check - if thumbnails, then no action, just show
        auto aitem = std::static_pointer_cast<CdsActiveItem>(obj);

        pugi::xml_document doc;
        auto root = doc.document_element();
        root.append_attribute(XML_DC_NAMESPACE_ATTR) = XML_DC_NAMESPACE;
        root.append_attribute(XML_UPNP_NAMESPACE_ATTR) = XML_UPNP_NAMESPACE;
        xmlBuilder->renderObject(obj, true, std::string::npos, &root);

        std::ostringstream buf;
        doc.print(buf, "", 0);

        std::string input = buf.str();
        std::string action = aitem->getAction();
        std::string output;

        log_debug("Script input: {}", input.c_str());
        if (strncmp(action.c_str(), "http://", 7) != 0) {
#ifdef TOMBDEBUG
            struct timespec before;
            getTimespecNow(&before);
#endif
            output = run_simple_process(config, action, "run", input);
#ifdef TOMBDEBUG
            long delta = getDeltaMillis(&before);
            log_debug("script executed in {} milliseconds", delta);
#endif
        } else {
            /// \todo actually fetch...
            log_debug("fetching {}", action.c_str());
            output = input;
        }
        log_debug("Script output: {}", output.c_str());

        auto clone = CdsObject::createObject(storage, objectType);
        aitem->copyTo(clone);

        UpnpXMLBuilder::updateObject(clone, output);

        if (!aitem->equals(clone, true)) // check for all differences
        {
            log_debug("Item changed, updating database");
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            updateManager->containerChanged(containerChanged);
            sessionManager->containerChangedUI(containerChanged);

            if (!aitem->equals(clone)) // check for visible differences
            {
                log_debug("Item changed visually, updating parent");
                updateManager->containerChanged(clone->getParentID(), FLUSH_ASAP);
            }
            obj = clone;
        } else {
            log_debug("Item untouched...");
        }
    }

    auto item = std::static_pointer_cast<CdsItem>(obj);

    fs::path path = item->getLocation();
    bool is_srt = false;

    std::string mimeType;
    std::string ext = getValueOrDefault(params, "ext");
    size_t edot = ext.rfind('.');
    if (edot != std::string::npos)
        ext = ext.substr(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi") || (ext == ".sub")) {
        // remove .ext
        std::string pathNoExt = path.parent_path() / path.stem();
        path = pathNoExt + ext;
        mimeType = MIMETYPE_TEXT;

        // reset resource id
        res_id = 0;
        is_srt = true;
    }

    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt)
            throw SubtitlesNotFoundException("Subtitle file " + path.string() + " is not available.");

        throw std::runtime_error("Failed to open " + path.string() + " - " + strerror(errno));
    }

    // for transcoded resourecs res_id will always be negative
    auto tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (!tr_profile.empty()) {
        if (res_id != -1)
            throw std::runtime_error("Invalid resource ID given!");
    } else {
        if (res_id == -1)
            throw std::runtime_error("Invalid resource ID given!");
    }
    log_debug("fetching resource id {}", res_id);

    // some resources are created dynamically and not saved in the database,
    // so we can not load such a resource for a particular item, we will have
    // to trust the resource handler parameter
    std::string rh = getValueOrDefault(params, RESOURCE_HANDLER);
    if (res_id > 0 && (res_id < item->getResourceCount() || !rh.empty())) {
        int res_handler;
        if (!rh.empty())
            res_handler = std::stoi(rh);
        else {
            auto resource = item->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            std::string protocolInfo = getValueOrDefault(item->getResource(res_id)->getAttributes(), "protocolInfo");
            if (!protocolInfo.empty()) {
                mimeType = getMTFromProtocolInfo(protocolInfo);
            }
        }

        auto h = MetadataHandler::createHandler(config, res_handler);
        if (mimeType.empty())
            mimeType = h->getMimeType();

        /* FIXME Upstream upnp / DNLA
#ifdef EXTEND_PROTOCOLINFO
        header = getDLNAtransferHeader(mimeType, header);
#endif

        if (!header.empty())
                info->http_header = ixmlCloneDOMString(header.c_str());
        */

        //info->content_type = ixmlCloneDOMString(mimeType.c_str());
        //auto io_handler = h->serveContent(item, res_id, &(info->file_length));

        auto io_handler = h->serveContent(item, res_id);
        io_handler->open(mode);
        log_debug("end");
        return io_handler;
    }

    if (!is_srt && !tr_profile.empty()) {
        std::string range = getValueOrDefault(params, "range");

        auto tr_d = std::make_unique<TranscodeDispatcher>(config, content);
        auto tp = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)
                      ->getByName(tr_profile);
        return tr_d->open(tp, path, item, range);
    }

    if (mimeType.empty())
        mimeType = item->getMimeType();

    /* FIXME Upstream headers / DNLA
    info->file_length = statbuf.st_size;
    info->content_type = ixmlCloneDOMString(mimeType.c_str());

    log_debug("Adding content disposition header: {}",
              header.c_str());
    // if we are dealing with a regular file we should add the
    // Accept-Ranges: bytes header, in order to indicate that we support
    // seeking
    if (S_ISREG(statbuf.st_mode))
    {
        if (!header.empty())
            header = header + "\r\n";

         header = header + "Accept-Ranges: bytes";
    }

    header = getDLNAtransferHeader(mimeType, header);

    if (!header.empty())
        info->http_header = ixmlCloneDOMString(header.c_str());
    */

    auto io_handler = std::make_unique<FileIOHandler>(path);
    io_handler->open(mode);
    content->triggerPlayHook(obj);
    log_debug("end");
    return io_handler;
}
