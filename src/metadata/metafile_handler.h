/*GRB*

    Gerbera - https://gerbera.io/

    metafile_handler.h - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file metadata/metafile_handler.h
/// @brief Definition of the MetaFileHandler and CueSheetHandler classes.

#ifndef __METAFILE_HANDLER_H__
#define __METAFILE_HANDLER_H__

#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "metacontent_handler.h"
#include "metadata_enums.h"

/// @brief This class is responsible for populating metadata from additional files
class MetaFileHandler : public MetacontentHandler {
public:
    explicit MetaFileHandler(const std::shared_ptr<Context>& context, std::shared_ptr<Content> content);
    bool fillMetadata(
        const std::shared_ptr<CdsObject>& obj,
        std::vector<int>& newIds) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;

private:
    static std::unique_ptr<ContentPathSetup> setup;
    std::shared_ptr<Content> content;
};

/// @brief This class is responsible for create entries from cuesheets
class CueSheetHandler : public MetadataHandler {
public:
    explicit CueSheetHandler(const std::shared_ptr<Context>& context, std::shared_ptr<Content> content);
    bool fillMetadata(
        const std::shared_ptr<CdsObject>& obj,
        std::vector<int>& newIds) override;
    std::unique_ptr<IOHandler> serveContent(
        const std::shared_ptr<CdsObject>& obj,
        const std::shared_ptr<CdsResource>& resource) override;
    /// @brief check whether file type is supported by handler
    bool isSupported(
        const std::string& contentType,
        bool isOggTheora,
        const std::string& mimeType,
        ObjectType mediaType) override { return contentType == CONTENT_TYPE_CUESHEET; }

private:
    std::shared_ptr<Content> content;
};

#endif // __METAFILE_HANDLER_H__
