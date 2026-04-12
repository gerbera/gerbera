/*GRB*

    Gerbera - https://gerbera.io/

    metafile_handler.cc - this file is part of Gerbera.

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

/// @file metadata/metafile_handler.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "metafile_handler.h" // API

#include "cds/cds_objects.h"
#include "content/content.h"
#include "iohandler/io_handler.h"

#include <vector>

std::unique_ptr<ContentPathSetup> MetaFileHandler::setup {};

MetaFileHandler::MetaFileHandler(
    const std::shared_ptr<Context>& context,
    std::shared_ptr<Content> content)
    : MetacontentHandler(context)
    , content(std::move(content))
{
    if (!setup) {
        setup = std::make_unique<ContentPathSetup>(config, database, definition, ConfigVal::IMPORT_RESOURCES_METAFILE_FILE_LIST, ConfigVal::IMPORT_RESOURCES_METAFILE_DIR_LIST);
    }
}

bool MetaFileHandler::fillMetadata(
    const std::shared_ptr<CdsObject>& obj,
    std::vector<int>& newIds)
{
    bool result = false;
#ifdef HAVE_JS
    auto pathList = setup->getContentPath(obj, SETTING_METAFILE);

    if (pathList.empty() || pathList[0].empty())
        obj->removeResource(ContentHandler::METAFILE);

    for (auto&& path : pathList) {
        if (!path.empty()) {
            log_debug("Running metafile handler on {} -> '{}'", obj->getLocation().c_str(), path.c_str());
            content->parseMetafile(obj, path);
            result = true;
        }
    }
#endif
    return result;
}

std::unique_ptr<IOHandler> MetaFileHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    return {};
}

CueSheetHandler::CueSheetHandler(
    const std::shared_ptr<Context>& context,
    std::shared_ptr<Content> content)
    : MetadataHandler(context)
    , content(std::move(content))
{
}

bool CueSheetHandler::fillMetadata(
    const std::shared_ptr<CdsObject>& obj,
    std::vector<int>& newIds)
{
    bool result = false;
#ifdef HAVE_JS
    auto cueIds = content->parseCueSheet(obj, obj->getLocation());
    newIds.insert(newIds.end(), cueIds.begin(), cueIds.end());
    result = true;
#endif
    return result;
}

std::unique_ptr<IOHandler> CueSheetHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    return {};
}
