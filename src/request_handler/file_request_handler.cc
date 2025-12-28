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

/// @file request_handler/file_request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::requests

#include "file_request_handler.h" // API

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/transcoding.h"
#include "config/setup/config_setup_path.h"
#include "content/content.h"
#include "database/database.h"
#include "database/db_param.h"
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

#ifdef HAVE_ZIP
#include <condition_variable>
#include <libzippp/libzippp.h>
#include <mutex>
#include <thread>
#endif

#include <sys/stat.h>
#include <unistd.h>

FileRequestHandler::FileRequestHandler(
    const std::shared_ptr<Content>& content,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks,
    std::shared_ptr<MetadataService> metadataService)
    : RequestHandler(content, xmlBuilder, quirks)
    , metadataService(std::move(metadataService))
{
}

bool FileRequestHandler::getInfo(
    const char* filename,
    UpnpFileInfo* info)
{
    log_debug("Start: {}", filename);

    auto params = URLUtils::parseParameters(filename, LINK_FILE_REQUEST_HANDLER);
    auto obj = loadObject(params);

    auto resourceId = parseResourceInfo(params);
    std::string zipRequest = getValueOrDefault(params, URL_PARAM_ZIP_REQUEST);

    if (!obj->isItem() && obj->getResourceCount() == 0 && zipRequest.empty()) {
        throw_std_runtime_error("Requested object {} is not an item or has no resources", filename);
    }

    // for transcoded resources res_id will always be negative
    std::string trProfile = getValueOrDefault(params, URL_PARAM_TRANSCODE_PROFILE_NAME);

    if (resourceId >= obj->getResourceCount() && trProfile.empty() && zipRequest.empty()) {
        throw_std_runtime_error("Requested resource {} does not exist", resourceId);
    }

    std::string mimeType;
    auto path = (obj->isItem() || !obj->isVirtual()) ? obj->getLocation() : "";
    Headers headers;

    if (obj->isItem() && !trProfile.empty())
        mimeType = getTranscodingInfo(obj, info, path, trProfile);
    else if (obj->isContainer() && !zipRequest.empty())
        mimeType = getZipInfo(obj, info);
    else
        mimeType = getResourceInfo(obj, info, resourceId, path, headers);

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

std::string FileRequestHandler::getTranscodingInfo(
    const std::shared_ptr<CdsObject>& obj,
    UpnpFileInfo* info,
    const std::string& path,
    const std::string& trProfile)
{
    auto transcodingProfile = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)->getByName(trProfile);
    if (!transcodingProfile)
        throw_std_runtime_error("Requested transcoding of file {} but no profile matching the name {} found", path, trProfile);

    std::string mimeType = transcodingProfile->getTargetMimeType();
    auto mimeProperties = transcodingProfile->getTargetMimeProperties();
    if (!mimeProperties.empty()) {
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
    return mimeType;
}

std::string FileRequestHandler::getZipInfo(
    const std::shared_ptr<CdsObject>& obj,
    UpnpFileInfo* info)
{
    std::string mimeType = "application/zip";
#ifdef HAVE_ZIP
    UpnpFileInfo_set_IsReadable(info, true);
    UpnpFileInfo_set_FileLength(info, UPNP_USING_CHUNKED);
#else
    UpnpFileInfo_set_IsReadable(info, false);
#endif
    UpnpFileInfo_set_LastModified(info, obj->getMTime().count());
    UpnpFileInfo_set_IsDirectory(info, false);
    return mimeType;
}

std::string FileRequestHandler::getResourceInfo(
    std::shared_ptr<CdsObject>& obj,
    UpnpFileInfo* info,
    std::size_t resourceId,
    std::string path,
    Headers& headers)
{
    UpnpFileInfo_set_IsReadable(info, false);
    UpnpFileInfo_set_LastModified(info, obj->getMTime().count());
    UpnpFileInfo_set_IsDirectory(info, true);

    auto resource = obj->getResource(resourceId);

    // Check if the resource is actually another external file, and if it exists
    bool isResourceFile = false;

    auto resPath = resource->getAttribute(ResourceAttribute::RESOURCE_FILE);
    isResourceFile = !resPath.empty() && resPath != obj->getLocation();
    if (isResourceFile) {
        log_debug("Resource is file: {}", path);
        path = resPath;
    }

    // Check the path (if its real) is accessible
    if (!path.empty()) {
        struct stat statbuf { };
        int ret = stat(path.c_str(), &statbuf);
        if (ret != 0) {
            if (isResourceFile) {
                throw ResourceNotFoundException(fmt::format("{} file {} is not available.", EnumMapper::getPurposeDisplay(resource->getPurpose()), path.c_str()));
            }
            throw_fmt_system_error("Failed to open {}", path);
        }

        // If we get to here we can read the thing
        UpnpFileInfo_set_IsReadable(info, true);
        UpnpFileInfo_set_LastModified(info, statbuf.st_mtime);
        bool isDir = (resource->getHandlerType() == ContentHandler::DEFAULT && S_ISDIR(statbuf.st_mode));
        UpnpFileInfo_set_IsDirectory(info, isDir);
        if (!isDir)
            UpnpFileInfo_set_FileLength(info, statbuf.st_size);
    }

    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    std::string mimeType = item ? item->getMimeType() : "";

    if (resource->getHandlerType() != ContentHandler::DEFAULT) {
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

        log_debug("getInfo {}:{}", obj->getID(), resource->getResId());
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
                log_debug("info {}={}", obj->getID(), size);
                UpnpFileInfo_set_FileLength(info, size);
            } else {
                UpnpFileInfo_set_FileLength(info, 0);
            }
        }
    } else if (item) {
        if (quirks)
            quirks->addCaptionInfo(item, headers);

        // Generate DNLA Headers
        auto mappings = config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string dlnaContentHeader = xmlBuilder->getDLNAContentHeader(getValueOrDefault(mappings, mimeType), resource, quirks);
        if (!dlnaContentHeader.empty()) {
            headers.addHeader(UPNP_DLNA_CONTENT_FEATURES_HEADER, dlnaContentHeader);
        }
    }

    if (mimeType.empty() && item)
        mimeType = item->getMimeType();

    return mimeType;
}

std::shared_ptr<MetadataHandler> FileRequestHandler::getResourceMetadataHandler(
    std::shared_ptr<CdsObject>& obj,
    std::shared_ptr<CdsResource>& resource) const
{
    log_debug("start: {}", obj->getID());
    auto resHandler = resource->getHandlerType();
    if (resource->getAttribute(ResourceAttribute::RESOURCE_FILE).empty()) {
        auto objID = stoiString(resource->getAttribute(ResourceAttribute::FANART_OBJ_ID));
        auto resID = stoiString(resource->getAttribute(ResourceAttribute::FANART_RES_ID));
        try {
            auto resObj = (objID > 0 && objID != obj->getID()) ? database->loadObject(objID) : nullptr;
            if (resObj) {
                auto resRes = resObj->getResource(resID);
                if (resRes) {
                    log_debug("Switch obj {}:{} to {}:{}", obj->getID(), resource->getResId(), objID, resID);
                    obj = resObj;
                    resource = resRes;
                    return getResourceMetadataHandler(obj, resource);
                }
            }
        } catch (const std::runtime_error& ex) {
            log_error(ex.what());
        }
    }
    log_debug("end: {}", obj->getID());
    return metadataService->getHandler(resHandler);
}

#ifdef HAVE_ZIP
class ContainerProgressListener : public libzippp::ZipProgressListener {
private:
    bool done { false };
    std::condition_variable cond;

public:
    ContainerProgressListener() { }
    virtual ~ContainerProgressListener() { }

    void wait(std::unique_lock<std::mutex>& lock)
    {
        if (!done)
            cond.wait(lock); // waiting for the task to complete
    }

    int cancel() override
    {
        return 0;
    }

    void progression(double p) override
    {
        log_debug("-- Progress {}: ", p);
        if (p == 1) {
            log_debug("-- Done {}: ", p);
            done = true;
            cond.notify_one();
        }
    }
};

static int archive(
    const fs::path& path,
    const std::map<std::string, fs::path>& content)
{
    log_debug("start: {}", path.string());
    libzippp::ZipArchive zipFile(path);
    zipFile.open(libzippp::ZipArchive::Write);
    // register the listener
    ContainerProgressListener cpl;
    std::mutex mutex;
    std::unique_lock<decltype(mutex)> lock(mutex);
    zipFile.addProgressListener(&cpl);

    if (!content.empty())
        zipFile.setProgressPrecision(1.0 / content.size());

    for (auto&& [name, path] : content) {
        log_debug("Add {} : {}", name, path.c_str());
        zipFile.addFile(fmt::format("{}{}", name, path.extension().c_str()).c_str(), path.c_str());
    }
    zipFile.close();

    log_debug("wait: {}", path.string());
    cpl.wait(lock);
    log_debug("end: {}", path.string());
    return 0;
}
#endif

std::unique_ptr<IOHandler> FileRequestHandler::openResource(
    std::shared_ptr<CdsObject>& obj,
    std::size_t resourceId)
{
    auto resource = obj->getResource(resourceId);
    auto metadataHandler = getResourceMetadataHandler(obj, resource);
    log_debug("serveContent {}:{}", obj->getID(), resource->getResId());
    return metadataHandler->serveContent(obj, resource);
}

std::unique_ptr<IOHandler> FileRequestHandler::openTranscoding(
    const std::shared_ptr<CdsObject>& obj,
    const std::string& path,
    const std::string& trProfile,
    const std::string& group,
    const std::map<std::string, std::string>& params)
{
    auto transcodingProfile = config->getTranscodingProfileListOption(ConfigVal::TRANSCODING_PROFILE_LIST)->getByName(trProfile);
    if (!transcodingProfile)
        throw_std_runtime_error("Requested transcoding of file {} but no profile matching the name {} found", path.c_str(), trProfile);

    std::string range = getValueOrDefault(params, "range");

    auto transcodeDispatcher = std::make_unique<TranscodeDispatcher>(content);
    return transcodeDispatcher->serveContent(transcodingProfile, path, obj, group, range);
}

std::unique_ptr<IOHandler> FileRequestHandler::openZip(
    const std::shared_ptr<CdsObject>& obj)
{
#ifdef HAVE_ZIP
    auto path = ConfigPathSetup::Home / fmt::format("Download-{}.zip", obj->getID());
    auto browseParam = BrowseParam(obj, BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS);
    auto result = database->browse(browseParam);
    std::map<std::string, fs::path> folderContent;
    log_debug("zipRequest: {}", result.size());
    for (auto&& cdsObj : result) {
        folderContent[cdsObj->getSortKey()] = cdsObj->getLocation();
    }
    try {
        archive(path, folderContent);
        return std::make_unique<ZipIOHandler>(path);
    } catch (std::runtime_error& ex) {
        log_debug(ex.what());
        return nullptr;
    }
#else
    return nullptr;
#endif
}

std::unique_ptr<IOHandler> FileRequestHandler::open(
    const char* filename,
    const std::shared_ptr<Quirks>& quirks,
    enum UpnpOpenFileMode mode)
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
    std::string zipRequest = getValueOrDefault(params, URL_PARAM_ZIP_REQUEST);
    // Serve metadata resources
    if (trProfile.empty() && zipRequest.empty() && obj->getResource(resourceId)->getHandlerType() != ContentHandler::DEFAULT) {
        return openResource(obj, resourceId);
    }

    auto path = (obj->isItem() || !obj->isVirtual()) ? obj->getLocation() : "";

    if (path.empty() && zipRequest.empty()) {
        throw_std_runtime_error("Object location for {} not set", filename);
    }

    auto it = params.find(CLIENT_GROUP_TAG);
    std::string group = DEFAULT_CLIENT_GROUP;
    if (it != params.end()) {
        group = it->second;
    }

    if (!trProfile.empty()) {
        return openTranscoding(obj, path, trProfile, group, params);
    }
    if (!zipRequest.empty()) {
        return openZip(obj);
    }
    content->triggerPlayHook(group, obj);

    // Boring old file
    return std::make_unique<FileIOHandler>(path);
}

std::size_t FileRequestHandler::parseResourceInfo(
    const std::map<std::string, std::string>& params)
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
