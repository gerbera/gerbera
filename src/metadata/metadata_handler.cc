/*GRB*

    Gerbera - https://gerbera.io/

    metadata_handler.cc - this file is part of Gerbera.

    Copyright (C) 2024-2026 Gerbera Contributors

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

/// @file metadata/metadata_handler.cc
#define GRB_LOG_FAC GrbLogFacility::metadata

#include "metadata_handler.h"

#include "cds/cds_item.h"
#include "config/config.h"
#include "config/config_val.h"
#include "context.h"
#include "util/tools.h"

MetadataHandler::MetadataHandler(const std::shared_ptr<Context>& context)
    : config(context->getConfig())
    , mime(context->getMime())
    , mimeContentTypeMappings(config->getDictionaryOption(ConfigVal::IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST))
{
}

MetadataHandler::~MetadataHandler() = default;

MediaMetadataHandler::MediaMetadataHandler(const std::shared_ptr<Context>& context, ConfigVal enableOption)
    : MetadataHandler(context)
    , enabled(this->config->getBoolOption(enableOption))
    , converterManager(context->getConverterManager())
{
}

MediaMetadataHandler::~MediaMetadataHandler() = default;

MediaMetadataHandler::MediaMetadataHandler(
    const std::shared_ptr<Context>& context,
    ConfigVal enableOption,
    ConfigVal enableContentOption,
    ConfigVal contentOption,
    ConfigVal metaOption,
    ConfigVal auxOption)
    : MetadataHandler(context)
    , enabled(this->config->getBoolOption(enableOption))
    , metaTags(this->config->getDictionaryOption(metaOption))
    , auxTags(this->config->getArrayOption(auxOption))
    , converterManager(context->getConverterManager())
    , enabledContentTypes(this->config->getBoolOption(enableContentOption))
    , contentTypes(this->config->getArrayOption(contentOption))
{
}

MediaMetadataHandler::MediaMetadataHandler(
    const std::shared_ptr<Context>& context,
    ConfigVal enableOption,
    ConfigVal enableContentOption,
    ConfigVal contentOption,
    ConfigVal metaOption,
    ConfigVal auxOption,
    ConfigVal enableCommentOption,
    ConfigVal commentOption)
    : MetadataHandler(context)
    , enabled(this->config->getBoolOption(enableOption))
    , isCommentEnabled(this->config->getBoolOption(enableCommentOption))
    , metaTags(this->config->getDictionaryOption(metaOption))
    , auxTags(this->config->getArrayOption(auxOption))
    , commentMap(this->config->getDictionaryOption(commentOption))
    , converterManager(context->getConverterManager())
    , enabledContentTypes(this->config->getBoolOption(enableContentOption))
    , contentTypes(this->config->getArrayOption(contentOption))
{
}

bool MediaMetadataHandler::isValidArtworkContentType(std::string_view artMimetype)
{
    // saw that simply "PNG" was used with some mp3's, so mimetype setting
    // was probably invalid
    return artMimetype.find('/') != std::string_view::npos;
}

std::shared_ptr<CdsResource> MediaMetadataHandler::addArtworkResource(
    const std::shared_ptr<CdsItem>& item,
    ContentHandler ch,
    const std::string& artMimetype)
{
    // if we could not determine the mimetype, then there is no
    // point to add the resource - it's probably garbage
    log_debug("Found artwork of type {} in file {}", artMimetype, item->getLocation().c_str());

    if (artMimetype != MIMETYPE_DEFAULT) {
        auto resource = std::make_shared<CdsResource>(ch, ResourcePurpose::Thumbnail);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(artMimetype));
        item->addResource(resource);
        return resource;
    }
    return {};
}

bool MediaMetadataHandler::isEnabled(const std::string& contentType)
{
    return enabled && (!enabledContentTypes || std::find(contentTypes.begin(), contentTypes.end(), contentType) != contentTypes.end());
}
