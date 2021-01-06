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
#include <unistd.h>
#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"
#include "database/database.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "update_manager.h"
#include "util/process.h"
#include "web/session_manager.h"

#include "util/tools.h"
#include "util/upnp_headers.h"
#include "util/upnp_quirks.h"

#include "transcoding/transcode_dispatcher.h"

FileRequestHandler::FileRequestHandler(std::shared_ptr<ContentManager> content, UpnpXMLBuilder* xmlBuilder)
    : RequestHandler(std::move(content))
    , xmlBuilder(xmlBuilder)
{
}

void FileRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("start");

    const struct sockaddr_storage* ctrlPtIPAddr = UpnpFileInfo_get_CtrlPtIPAddr(info);
    std::string userAgent = UpnpFileInfo_get_Os_cstr(info);
    auto quirks = std::make_unique<Quirks>(config, ctrlPtIPAddr, userAgent);

    auto headers = std::make_unique<Headers>();

    auto params = parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = getObjectById(params);

    if (!obj->isItem()) {
        throw_std_runtime_error("requested object is not an item");
    }

    auto item = std::static_pointer_cast<CdsItem>(obj);

    // determining which resource to serve
    size_t res_id = 0;
    auto res_id_it = params.find(URL_RESOURCE_ID);
    if (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID)
        res_id = std::stoi(res_id_it->second);
    else
        res_id = SIZE_MAX;

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

        throw_std_runtime_error("Failed to open " + path.string() + " - " + strerror(errno));
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
    std::string tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

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

        auto h = MetadataHandler::createHandler(config, mime, res_handler);
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
            throw_std_runtime_error("Transcoding of file " + path.string()
                + " but no profile matching the name "
                + tr_profile + " found");

        mimeType = tp->getTargetMimeType();

        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        if (getValueOrDefault(mappings, mimeType) == CONTENT_TYPE_PCM) {
            std::string freq = item->getResource(0)->getAttribute(R_SAMPLEFREQUENCY);
            std::string nrch = item->getResource(0)->getAttribute(R_NRAUDIOCHANNELS);
            if (!freq.empty())
                mimeType = mimeType + ";rate=" + freq;
            if (!nrch.empty())
                mimeType = mimeType + ";channels=" + nrch;
        }

        UpnpFileInfo_set_FileLength(info, -1);
    } else {
        UpnpFileInfo_set_FileLength(info, statbuf.st_size);

        quirks->addCaptionInfo(item, headers);

        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = getDLNAContentHeader(config, getValueOrDefault(mappings, item->getMimeType()));
        if (!dlnaContentHeader.empty()) {
            headers->addHeader(UPNP_DLNA_CONTENT_FEATURES_HEADER, dlnaContentHeader);
        }
    }

    if (mimeType.empty())
        mimeType = item->getMimeType();

    std::string dlnaTransferHeader = getDLNATransferHeader(config, mimeType);
    if (!dlnaTransferHeader.empty()) {
        headers->addHeader(UPNP_DLNA_TRANSFER_MODE_HEADER, dlnaTransferHeader);
    }

    //log_debug("sizeof off_t {}, statbuf.st_size {}", sizeof(off_t), sizeof(statbuf.st_size));
    //log_debug("getInfo: file_length: " OFF_T_SPRINTF "", statbuf.st_size);

    UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
    UpnpFileInfo_set_IsDirectory(info, S_ISDIR(statbuf.st_mode));
#if defined(USING_NPUPNP)
    UpnpFileInfo_set_ContentType(info, mimeType);
#else
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(mimeType.c_str()));
#endif

    headers->writeHeaders(info);

    // log_debug("getInfo: Requested {}, ObjectID: {}, Location: {}, MimeType: {}",
    //      filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("web_get_info(): end");
}

std::unique_ptr<IOHandler> FileRequestHandler::open(const char* filename, enum UpnpOpenFileMode mode)
{
    log_debug("start");

    // We explicitly do not support UPNP_WRITE due to security reasons.
    if (mode != UPNP_READ) {
        throw_std_runtime_error("UPNP_WRITE unsupported");
    }

    auto params = parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = getObjectById(params);

    if (!obj->isItem()) {
        throw_std_runtime_error("requested object is not an item");
    }

    auto item = std::static_pointer_cast<CdsItem>(obj);

    // determining which resource to serve
    size_t res_id = 0;
    auto res_id_it = params.find(URL_RESOURCE_ID);
    if (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID)
        res_id = std::stoi(res_id_it->second);
    else
        res_id = SIZE_MAX;

    fs::path path = item->getLocation();
    bool is_srt = false;

    std::string ext = getValueOrDefault(params, "ext");
    size_t edot = ext.rfind('.');
    if (edot != std::string::npos)
        ext = ext.substr(edot);
    if ((ext == ".srt") || (ext == ".ssa") || (ext == ".smi") || (ext == ".sub")) {
        // remove .ext
        std::string pathNoExt = path.parent_path() / path.stem();
        path = pathNoExt + ext;

        // reset resource id
        res_id = 0;
        is_srt = true;
    }

    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt)
            throw SubtitlesNotFoundException("Subtitle file " + path.string() + " is not available.");

        throw_std_runtime_error("Failed to open " + path.string() + " - " + strerror(errno));
    }

    // for transcoded resourecs res_id will always be negative
    auto tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (!tr_profile.empty()) {
        if (res_id != SIZE_MAX)
            throw_std_runtime_error("Invalid resource ID given");
    } else {
        if (res_id == SIZE_MAX)
            throw_std_runtime_error("Invalid resource ID given");
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
        }

        auto h = MetadataHandler::createHandler(config, mime, res_handler);
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

        auto io_handler = tr_d->serveContent(tp, path, item, range);
        io_handler->open(mode);

        log_debug("end");
        return io_handler;
    }

    auto io_handler = std::make_unique<FileIOHandler>(path);
    io_handler->open(mode);
    content->triggerPlayHook(obj);

    log_debug("end");
    return io_handler;
}
