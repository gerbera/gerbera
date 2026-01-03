/*MT*

    MediaTomb - http://www.mediatomb.cc/

    file_request_handler.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file request_handler/file_request_handler.h
/// @brief Definition of the FileRequestHandler class.
#ifndef __FILE_REQUEST_HANDLER_H__
#define __FILE_REQUEST_HANDLER_H__

#include "request_handler.h"

#include <memory>

#include "upnp/xml_builder.h"

class CdsObject;
class CdsResource;
class Headers;
class MetadataHandler;
class MetadataService;
enum class ResourcePurpose;

class FileRequestHandler : public RequestHandler {

public:
    explicit FileRequestHandler(
        const std::shared_ptr<Content>& content,
        const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
        const std::shared_ptr<Quirks>& quirks,
        std::shared_ptr<MetadataService> metadataService);

    /// \inherit
    bool getInfo(const char* filename, UpnpFileInfo* info) override;

    /// @brief Prepares the output buffer and calls the process function.
    /// @param filename Requested URL
    /// @param quirks allows modifying the content of the response based on the client
    /// @param mode either UPNP_READ or UPNP_WRITE
    /// @return the appropriate IOHandler for the request.
    std::unique_ptr<IOHandler> open(
        const char* filename,
        const std::shared_ptr<Quirks>& quirks,
        enum UpnpOpenFileMode mode) override;

private:
    static std::size_t parseResourceInfo(
        const std::map<std::string, std::string>& params);
    std::shared_ptr<MetadataHandler> getResourceMetadataHandler(
        std::shared_ptr<CdsObject>& obj,
        std::shared_ptr<CdsResource>& resource) const;

    /// @brief get header information from file
    void getFileInfo(
        const std::string& path,
        UpnpFileInfo* info,
        bool isResourceFile,
        ContentHandler handlerType,
        ResourcePurpose purpose);
    /// @brief get header information for transcoding
    std::string getTranscodingInfo(
        const std::shared_ptr<CdsObject>& obj,
        UpnpFileInfo* info,
        const std::string& path,
        const std::string& trProfile);
    /// @brief get header information for zip archives
    std::string getZipInfo(
        const std::shared_ptr<CdsObject>& obj,
        UpnpFileInfo* info);
    /// @brief get header information for resource
    std::string getResourceInfo(
        std::shared_ptr<CdsObject>& obj,
        UpnpFileInfo* info,
        std::size_t resourceId,
        std::string path,
        Headers& headers);

    /// @brief open resource or file stream
    std::unique_ptr<IOHandler> openResource(
        std::shared_ptr<CdsObject>& obj,
        std::size_t resourceId);
    /// @brief open transcoding stream
    std::unique_ptr<IOHandler> openTranscoding(
        const std::shared_ptr<CdsObject>& obj,
        const std::string& path,
        const std::string& trProfile,
        const std::string& group,
        const std::map<std::string, std::string>& params);
    /// @brief open zip archive stream
    std::unique_ptr<IOHandler> openZip(
        const std::shared_ptr<CdsObject>& obj);

    std::shared_ptr<MetadataService> metadataService;
};

#endif // __FILE_REQUEST_HANDLER_H__
