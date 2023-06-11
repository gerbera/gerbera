/*MT*

    MediaTomb - http://www.mediatomb.cc/

    file_request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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
#define LOG_FAC log_facility_t::requests

#include "file_request_handler.h" // API

#include <sys/stat.h>
#include <unistd.h>

#include "cds/cds_item.h"
#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcode_dispatcher.h"
#include "transcoding/transcoding.h"
#include "upnp_common.h"
#include "upnp_xml.h"
#include "util/grb_net.h"
#include "util/tools.h"
#include "util/upnp_headers.h"
#include "util/upnp_quirks.h"
#include "util/url_utils.h"
#include "web/session_manager.h"

FileRequestHandler::FileRequestHandler(const std::shared_ptr<ContentManager>& content, std::shared_ptr<UpnpXMLBuilder> xmlBuilder)
    : RequestHandler(content)
    , xmlBuilder(std::move(xmlBuilder))
{
}

void FileRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("Start: {}", filename);

    auto quirks = getQuirks(info);

    auto params = URLUtils::parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = loadObject(params);

    auto resourceId = parseResourceInfo(params);

    if (!obj->isItem() && obj->getResourceCount() == 0) {
        throw_std_runtime_error("Requested object {} is not an item and has no resources", filename);
    }

    // for transcoded resources res_id will always be negative
    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (resourceId >= obj->getResourceCount() && trProfile.empty()) {
        throw_std_runtime_error("Requested resource {} does not exist", resourceId);
    }
    auto resource = trProfile.empty() ? obj->getResource(resourceId) : std::make_shared<CdsResource>(ContentHandler::TRANSCODE, CdsResource::Purpose::Transcode);

    fs::path path = obj->getLocation();

    // Check if the resource is actually another external file, and if it exists
    bool isResourceFile = false;

    auto resPath = resource->getAttribute(CdsResource::Attribute::RESOURCE_FILE);
    isResourceFile = !resPath.empty() && resPath != obj->getLocation();
    if (isResourceFile) {
        log_debug("Resource is file: {}", path.string());
        path = resPath;
    }

    // Check the path (if its real) is accessible
    struct stat statbuf {
    };
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (isResourceFile) {
            throw SubtitlesNotFoundException(fmt::format("Subtitle file {} is not available.", path.c_str()));
        }
        throw_fmt_system_error("Failed to open {}", path.c_str());
    }

    // If we get to here we can read the thing
    UpnpFileInfo_set_IsReadable(info, true);
    UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
    UpnpFileInfo_set_IsDirectory(info, S_ISDIR(statbuf.st_mode));

    auto headers = Headers();
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    std::string mimeType = item ? item->getMimeType() : "";

    if (resource->getHandlerType() != ContentHandler::DEFAULT && resource->getHandlerType() != ContentHandler::TRANSCODE) {
        auto metadataHandler = getResourceMetadataHandler(obj, resource);

        std::string protocolInfo = resource->getAttribute(CdsResource::Attribute::PROTOCOLINFO);
        if (!protocolInfo.empty()) {
            mimeType = getMTFromProtocolInfo(protocolInfo);
        }
        if (mimeType.empty())
            mimeType = metadataHandler->getMimeType();

        auto ioHandler = metadataHandler->serveContent(obj, resource);

        if (ioHandler) {
            // Get size
            ioHandler->open(UPNP_READ);
            ioHandler->seek(0L, SEEK_END);
            off_t size = ioHandler->tell();
            ioHandler->close();
            UpnpFileInfo_set_FileLength(info, size);
        }

    } else if (!isResourceFile && !trProfile.empty()) {

        auto transcodingProfile = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(trProfile);
        if (!transcodingProfile)
            throw_std_runtime_error("Transcoding of file {} but no profile matching the name {} found", path.c_str(), trProfile);

        mimeType = transcodingProfile->getTargetMimeType();

        // TODO: this WAV specific logic should be more generic
        auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        if (getValueOrDefault(mappings, mimeType) == CONTENT_TYPE_PCM) {
            auto res = obj->getResource(ContentHandler::DEFAULT);
            std::string freq = res->getAttribute(CdsResource::Attribute::SAMPLEFREQUENCY);
            std::string nrch = res->getAttribute(CdsResource::Attribute::NRAUDIOCHANNELS);
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
    } else if (item) {
        UpnpFileInfo_set_FileLength(info, statbuf.st_size);
        quirks->addCaptionInfo(item, headers);
        resource = item->getResource(resourceId);

        // Generate DNLA Headers
        auto mappings = config->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = xmlBuilder->getDLNAContentHeader(getValueOrDefault(mappings, mimeType), resource);
        if (!dlnaContentHeader.empty()) {
            headers.addHeader(UPNP_DLNA_CONTENT_FEATURES_HEADER, dlnaContentHeader);
        }
    }

    if (mimeType.empty() && item)
        mimeType = item->getMimeType();

    std::string dlnaTransferHeader = xmlBuilder->getDLNATransferHeader(mimeType);
    if (!dlnaTransferHeader.empty()) {
        headers.addHeader(UPNP_DLNA_TRANSFER_MODE_HEADER, dlnaTransferHeader);
    }

#ifdef USING_NPUPNP
    info->content_type = std::move(mimeType);
#else
    UpnpFileInfo_set_ContentType(info, mimeType.c_str());
#endif

    headers.writeHeaders(info);

    // log_debug("getInfo: Requested {}, ObjectID: {}, Location: {}, MimeType: {}",
    //      filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("end: {}", filename);
}

std::unique_ptr<MetadataHandler> FileRequestHandler::getResourceMetadataHandler(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource) const
{
    auto resHandler = resource->getHandlerType();
    return MetadataHandler::createHandler(context, content, resHandler);
}

std::unique_ptr<IOHandler> FileRequestHandler::open(const char* filename, enum UpnpOpenFileMode mode)
{
    log_debug("start: {}", filename);

    // We explicitly do not support UPNP_WRITE due to security reasons.
    if (mode != UPNP_READ) {
        throw_std_runtime_error("UPNP_WRITE unsupported");
    }

    auto params = URLUtils::parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = loadObject(params);
    auto resourceId = parseResourceInfo(params);

    // Transcoding
    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    // Serve metadata resources
    if (trProfile.empty() && obj->getResource(resourceId)->getHandlerType() != ContentHandler::DEFAULT) {
        auto resource = obj->getResource(resourceId);
        auto metadataHandler = getResourceMetadataHandler(obj, resource);
        return metadataHandler->serveContent(obj, resource);
    }

    auto path = obj->getLocation();

    auto it = params.find(CLIENT_GROUP_TAG);
    std::string group = DEFAULT_CLIENT_GROUP;
    if (it != params.end()) {
        group = it->second;
    }

    if (!trProfile.empty()) {
        auto transcodingProfile = config->getTranscodingProfileListOption(CFG_TRANSCODING_PROFILE_LIST)->getByName(trProfile);
        if (!transcodingProfile)
            throw_std_runtime_error("Transcoding of file {} but no profile matching the name {} found", path.c_str(), trProfile);

        std::string range = getValueOrDefault(params, "range");

        auto transcodeDispatcher = std::make_unique<TranscodeDispatcher>(content);
        return transcodeDispatcher->serveContent(transcodingProfile, path, obj, group, range);
    }

    content->triggerPlayHook(group, obj);

    // Boring old file
    return std::make_unique<FileIOHandler>(path);
}

std::size_t FileRequestHandler::parseResourceInfo(std::map<std::string, std::string>& params)
{
    std::size_t resourceId = 0;
    try {
        auto resIdParam = params.at(URL_RESOURCE_ID);
        if (resIdParam != URL_VALUE_TRANSCODE_NO_RES_ID) {
            resourceId = stoiString(resIdParam);
        }
    } catch (const std::out_of_range&) {
    }

    log_debug("Resource ID: {}", resourceId);

    return resourceId;
}

std::unique_ptr<Quirks> FileRequestHandler::getQuirks(const UpnpFileInfo* info) const
{
    auto ctrlPtIPAddr = std::make_shared<GrbNet>(UpnpFileInfo_get_CtrlPtIPAddr(info));
    // HINT: most clients do not report exactly the same User-Agent for UPnP services and file request.
    std::string userAgent = UpnpFileInfo_get_Os_cstr(info);
    return std::make_unique<Quirks>(xmlBuilder, context->getClients(), ctrlPtIPAddr, std::move(userAgent));
}
