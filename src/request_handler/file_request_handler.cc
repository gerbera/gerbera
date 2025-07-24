/*MT*

    MediaTomb - http://www.mediatomb.cc/

    file_request_handler.cc - this file is part of MediaTomb.

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

/// \file file_request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#include "file_request_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/transcoding.h"
#include "content/content.h"
#include "database/database.h"
#include "exceptions.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "metadata/metadata_service.h"
#include "transcoding/transcode_dispatcher.h"
#include "upnp/compat.h"
#include "upnp/headers.h"
#include "upnp/quirks.h"
#include "upnp/upnp_common.h"
#include "upnp/xml_builder.h"
#include "util/grb_net.h"
#include "util/tools.h"
#include "util/url_utils.h"
#include "web/session_manager.h"

#include <sys/stat.h>
#include <unistd.h>

FileRequestHandler::FileRequestHandler(const std::shared_ptr<Content>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder, const std::shared_ptr<Quirks>& quirks,
    std::shared_ptr<MetadataService> metadataService)
    : RequestHandler(content, xmlBuilder, quirks)
    , metadataService(std::move(metadataService))
{
}

bool FileRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    log_debug("Start: {}", filename);

    auto params = URLUtils::parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = loadObject(params);

    auto resourceId = parseResourceInfo(params);

    if (!obj->isItem() && obj->getResourceCount() == 0) {
        throw_std_runtime_error("Requested object {} is not an item or has no resources", filename);
    }

    // for transcoded resources res_id will always be negative
    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);
    if (resourceId >= obj->getResourceCount() && trProfile.empty()) {
        throw_std_runtime_error("Requested resource {} does not exist", resourceId);
    }
    auto resource = trProfile.empty() ? obj->getResource(resourceId) : std::make_shared<CdsResource>(ContentHandler::TRANSCODE, ResourcePurpose::Transcode);

    auto path = (obj->isItem() || !obj->isVirtual()) ? obj->getLocation() : "";

    // Check if the resource is actually another external file, and if it exists
    bool isResourceFile = false;

    auto resPath = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
    isResourceFile = !resPath.empty() && resPath != obj->getLocation();
    if (isResourceFile) {
        log_debug("Resource is file: {}", path.string());
        path = resPath;
    }

    // Check the path (if its real) is accessible
    if (!path.empty()) {
        struct stat statbuf {
        };
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0) {
            if (isResourceFile) {
                throw ResourceNotFoundException(fmt::format("{} file {} is not available.", EnumMapper::getPurposeDisplay(resource->getPurpose()), path.c_str()));
            }
            throw_fmt_system_error("Failed to open {}", path.c_str());
        }

        // If we get to here we can read the thing
        UpnpFileInfo_set_IsReadable(info, true);
        UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
        bool isDir = (resource->getHandlerType() == ContentHandler::DEFAULT && S_ISDIR(statbuf.st_mode));
        UpnpFileInfo_set_IsDirectory(info, isDir);
        if (!isDir)
            UpnpFileInfo_set_FileLength(info, statbuf.st_size);
    } else {
        UpnpFileInfo_set_IsReadable(info, false);
        UpnpFileInfo_set_LastModified(info, obj->getMTime().count());
        UpnpFileInfo_set_IsDirectory(info, true);
    }

    auto headers = Headers();
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    std::string mimeType = item ? item->getMimeType() : "";

    if (resource->getHandlerType() != ContentHandler::DEFAULT && resource->getHandlerType() != ContentHandler::TRANSCODE) {
        auto metadataHandler = getResourceMetadataHandler(obj, resource);

        std::string protocolInfo = resource->getAttribute(ResourceAttribute::PROTOCOLINFO);
        if (!protocolInfo.empty()) {
            mimeType = getMTFromProtocolInfo(protocolInfo);
        }
        if (mimeType.empty()) {
            mimeType = metadataHandler->getMimeType();
        }
        if (startswith(mimeType, "text"))
            mimeType = fmt::format("{}; charset=utf-8", mimeType);

        auto resSize = resource->getAttribute(ResourceAttribute::SIZE);
        if (!resSize.empty()) {
            UpnpFileInfo_set_FileLength(info, stoiString(resSize));
        } else {
            auto ioHandler = metadataHandler->serveContent(obj, resource);

            if (ioHandler) {
                // Get size
                ioHandler->open(UPNP_READ);
                ioHandler->seek(0L, SEEK_END);
                off_t size = ioHandler->tell();
                ioHandler->close();
                UpnpFileInfo_set_FileLength(info, size);
            } else {
                UpnpFileInfo_set_FileLength(info, 0);
            }
        }
    } else if (!isResourceFile && !trProfile.empty()) {

        auto transcodingProfile = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)->getByName(trProfile);
        if (!transcodingProfile)
            throw_std_runtime_error("Transcoding of file {} but no profile matching the name {} found", path.c_str(), trProfile);

        mimeType = transcodingProfile->getTargetMimeType();
        auto mimeProperties = transcodingProfile->getTargetMimeProperties();
        if (mimeProperties.size() > 0) {
            auto res = obj->getResource(ContentHandler::DEFAULT);
            std::vector<std::string> propList = { mimeType };
            for (const auto& prop : mimeProperties) {
                std::string value;
                if (prop.getAttribute() != ResourceAttribute::MAX)
                    value = res->getAttribute(prop.getAttribute());
                if (prop.getMetadata() != MetadataFields::M_MAX)
                    value = obj->getMetaData(prop.getMetadata());
                if (!value.empty())
                    propList.push_back(fmt::format("{}={}", prop.getKey(), value));
            }
            mimeType = fmt::format("{}", fmt::join(propList, ";"));
        }

        UpnpFileInfo_set_FileLength(info, UPNP_USING_CHUNKED);
    } else if (item) {
        quirks->addCaptionInfo(item, headers);
        resource = item->getResource(resourceId);

        // Generate DNLA Headers
        auto mappings = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = xmlBuilder->getDLNAContentHeader(getValueOrDefault(mappings, mimeType), resource, quirks);
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

    GrbUpnpFileInfoSetContentType(info, mimeType);

    if (quirks)
        quirks->updateHeaders(headers);
    headers.writeHeaders(info);

    // log_debug("getInfo: Requested {}, ObjectID: {}, Location: {}, MimeType: {}",
    //      filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("end: {}", filename);
    return quirks && quirks->getClient();
}

std::shared_ptr<MetadataHandler> FileRequestHandler::getResourceMetadataHandler(std::shared_ptr<CdsObject>& obj, std::shared_ptr<CdsResource>& resource) const
{
    auto resHandler = resource->getHandlerType();
    if (resource->getAttribute(ResourceAttribute::RESOURCE_FILE).empty()) {
        auto objID = stoiString(resource->getAttribute(ResourceAttribute::FANART_OBJ_ID));
        auto resID = stoiString(resource->getAttribute(ResourceAttribute::FANART_RES_ID));
        try {
            auto resObj = (objID > 0 && objID != obj->getID()) ? database->loadObject(objID) : nullptr;
            if (resObj) {
                auto resRes = resObj->getResource(resID);
                if (resRes) {
                    obj = resObj;
                    resource = resRes;
                    return getResourceMetadataHandler(obj, resource);
                }
            }
        } catch (const std::runtime_error& ex) {
            log_error(ex.what());
        }
    }
    return metadataService->getHandler(resHandler);
}

std::unique_ptr<IOHandler> FileRequestHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
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

    auto path = (obj->isItem() || !obj->isVirtual()) ? obj->getLocation() : "";

    if (path.empty()) {
        throw_std_runtime_error("Object location for {} not set", filename);
    }

    auto it = params.find(CLIENT_GROUP_TAG);
    std::string group = DEFAULT_CLIENT_GROUP;
    if (it != params.end()) {
        group = it->second;
    }

    if (!trProfile.empty()) {
        auto transcodingProfile = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)->getByName(trProfile);
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

std::size_t FileRequestHandler::parseResourceInfo(const std::map<std::string, std::string>& params)
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
