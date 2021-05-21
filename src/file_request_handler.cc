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

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcode_dispatcher.h"
#include "util/process.h"
#include "util/tools.h"
#include "util/upnp_headers.h"
#include "util/upnp_quirks.h"
#include "web/session_manager.h"

FileRequestHandler::FileRequestHandler(std::shared_ptr<ContentManager> content, UpnpXMLBuilder* xmlBuilder)
    : RequestHandler(std::move(content))
    , xmlBuilder(xmlBuilder)
{
}

static bool checkFileAndSubtitle(fs::path& path, const std::shared_ptr<CdsObject>& obj, const size_t& res_id, std::string& mimeType, struct stat& statbuf, const std::string& rh)
{
    bool is_srt = false;

    if (!rh.empty()) {
        auto res_path = obj->getResource(res_id)->getAttribute(R_RESOURCE_FILE);
        mimeType = MIMETYPE_TEXT;
        is_srt = !res_path.empty();
        if (is_srt) {
            path = res_path;
        }
    }
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (is_srt) {
            throw SubtitlesNotFoundException(fmt::format("Subtitle file {} is not available.", path.c_str()));
        }
        throw_std_runtime_error("Failed to open {}: {}", path.c_str(), std::strerror(errno));
    }
    return is_srt;
}

void FileRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("start");

    const struct sockaddr_storage* ctrlPtIPAddr = UpnpFileInfo_get_CtrlPtIPAddr(info);
    // HINT: most clients do not report exactly the same User-Agent for UPnP services and file request.
    std::string userAgent = UpnpFileInfo_get_Os_cstr(info);
    auto quirks = std::make_unique<Quirks>(context, ctrlPtIPAddr, userAgent);

    auto headers = std::make_unique<Headers>();

    auto params = parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = getObjectById(params);
    std::string rh = getValueOrDefault(params, RESOURCE_HANDLER);

    // determining which resource to serve
    auto res_id_it = params.find(URL_RESOURCE_ID);
    size_t res_id = (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID) ? std::stoi(res_id_it->second) : std::numeric_limits<std::size_t>::max();

    if (!obj->isItem() && rh.empty()) {
        throw_std_runtime_error("Requested object {} is not an item", filename);
    }

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);

    fs::path path = item != nullptr ? item->getLocation() : "";
    std::string mimeType;
    struct stat statbuf;
    bool is_srt = checkFileAndSubtitle(path, obj, res_id, mimeType, statbuf, rh);

    UpnpFileInfo_set_IsReadable(info, access(path.c_str(), R_OK) == 0);

    std::string header;
    log_debug("path: {}", path.c_str());
    if (!path.filename().empty()) {
        header = fmt::format("Content-Disposition: attachment; filename=\"{}\"", path.filename().c_str());
    }

    // for transcoded resourecs res_id will always be negative
    std::string tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

    log_debug("fetching resource id {}", res_id);

    // some resources are created dynamically and not saved in the database,
    // so we can not load such a resource for a particular item, we will have
    // to trust the resource handler parameter
    if ((res_id > 0 && res_id < obj->getResourceCount()) || !rh.empty()) {
        int res_handler;
        if (!rh.empty())
            res_handler = std::stoi(rh);
        else {
            auto resource = obj->getResource(res_id);
            res_handler = resource->getHandlerType();
            // http-get:*:image/jpeg:*
            std::string protocolInfo = getValueOrDefault(obj->getResource(res_id)->getAttributes(), "protocolInfo");
            if (!protocolInfo.empty()) {
                mimeType = getMTFromProtocolInfo(protocolInfo);
            }
        }

        auto h = MetadataHandler::createHandler(context, res_handler);
        if (mimeType.empty())
            mimeType = h->getMimeType();

        auto io_handler = h->serveContent(obj, res_id);

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
            throw_std_runtime_error("Transcoding of file {} but no profile matching the name {} found", path.c_str(), tr_profile.c_str());

        mimeType = tp->getTargetMimeType();

        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        if (getValueOrDefault(mappings, mimeType) == CONTENT_TYPE_PCM) {
            std::string freq = obj->getResource(0)->getAttribute(R_SAMPLEFREQUENCY);
            std::string nrch = obj->getResource(0)->getAttribute(R_NRAUDIOCHANNELS);
            if (!freq.empty())
                mimeType += fmt::format(";rate={}", freq);
            if (!nrch.empty())
                mimeType += fmt::format(";channels={}", nrch);
        }

#ifdef UPNP_USING_CHUNKED
        UpnpFileInfo_set_FileLength(info, UPNP_USING_CHUNKED);
#else
        UpnpFileInfo_set_FileLength(info, -1);
#endif
    } else if (item != nullptr) {
        UpnpFileInfo_set_FileLength(info, statbuf.st_size);

        quirks->addCaptionInfo(item, headers);

        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = getDLNAContentHeader(config, getValueOrDefault(mappings, item->getMimeType()));
        if (!dlnaContentHeader.empty()) {
            headers->addHeader(UPNP_DLNA_CONTENT_FEATURES_HEADER, dlnaContentHeader);
        }
    }

    if (mimeType.empty() && item != nullptr)
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
    std::string rh = getValueOrDefault(params, RESOURCE_HANDLER);

    // determining which resource to serve
    auto res_id_it = params.find(URL_RESOURCE_ID);
    size_t res_id = (res_id_it != params.end() && res_id_it->second != URL_VALUE_TRANSCODE_NO_RES_ID) ? std::stoi(res_id_it->second) : std::numeric_limits<std::size_t>::max();

    if (!obj->isItem() && rh.empty()) {
        throw_std_runtime_error("requested object {} is not an item", filename);
    }

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);

    fs::path path = item != nullptr ? item->getLocation() : "";
    std::string mimeType;
    struct stat statbuf;
    bool is_srt = checkFileAndSubtitle(path, obj, res_id, mimeType, statbuf, rh);

    // for transcoded resourecs res_id will always be negative
    auto tr_profile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (!tr_profile.empty()) {
        if (res_id != std::numeric_limits<std::size_t>::max())
            throw_std_runtime_error("Invalid resource ID given");
    } else {
        if (res_id == std::numeric_limits<std::size_t>::max())
            throw_std_runtime_error("Invalid resource ID given");
    }
    log_debug("fetching resource id {}", res_id);

    // some resources are created dynamically and not saved in the database,
    // so we can not load such a resource for a particular item, we will have
    // to trust the resource handler parameter
    if ((res_id > 0 && res_id < obj->getResourceCount()) || !rh.empty()) {
        auto res_handler = int { !rh.empty() ? std::stoi(rh) : obj->getResource(res_id)->getHandlerType() };

        auto h = MetadataHandler::createHandler(context, res_handler);
        auto io_handler = h->serveContent(obj, res_id);
        io_handler->open(mode);

        log_debug("end");
        return io_handler;
    }

    if (!is_srt && !tr_profile.empty()) {
        std::string range = getValueOrDefault(params, "range");

        auto tr_d = std::make_unique<TranscodeDispatcher>(content);
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
    return std::move(io_handler);
}
